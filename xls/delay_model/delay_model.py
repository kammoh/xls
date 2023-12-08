# Copyright 2020 The XLS Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# pylint: disable=g-long-lambda
"""Delay model for XLS operations.

The delay model estimates the delay (latency) of XLS operations when synthesized
in hardware. The delay model can both generates C++ code to compute delay as
well as provide delay estimates in Python.
"""

import abc
import dataclasses
import random

from typing import Sequence, List, Tuple, Callable

import numpy as np
from scipy import optimize as opt

from xls.delay_model import delay_model_pb2


class Error(Exception):
  pass


@dataclasses.dataclass
class RawDataPoint:
  """Measurements used by RegressionEstimator and BoundingBoxEstimator."""
  delay_factors: List[int]
  delay_ps: int


class Estimator(metaclass=abc.ABCMeta):
  """Base class for delay estimators.

    An Estimator provides and estimate of XLS operation delay based on
    parameters of the operation.

    Attributes:
      op: The XLS op modeled by this delay estimator. The value should
        match the name of the XLS Op enum value.  Example: 'kAdd'.
  """

  def __init__(self, op: str):
    self.op = op

  @abc.abstractmethod
  def cpp_delay_code(self, node_identifier: str) -> str:
    """Returns the sequence of C++ statements which compute the delay.

    Args:
      node_identifier: The string identifier of the Node* value whose delay is
        being estimated..

    Returns:
      Sequence of C++ statements to compute the delay. The delay
      should be returned as an int64_t in the C++ code. For example:

        if (node->BitCountOrDie() == 1) { return 0; }
        return 2 * node->operand_count();
    """
    raise NotImplementedError

  @abc.abstractmethod
  def operation_delay(self, operation: delay_model_pb2.Operation) -> int:
    """Returns the estimated delay for the given operation."""
    raise NotImplementedError


class FixedEstimator(Estimator):
  """A delay estimator which always returns a fixed delay."""

  def __init__(self, op, delay: int):
    super(FixedEstimator, self).__init__(op)
    self.fixed_delay = delay

  def operation_delay(self, operation: delay_model_pb2.Operation) -> int:
    return self.fixed_delay

  def cpp_delay_code(self, node_identifier: str) -> str:
    return 'return {};'.format(self.fixed_delay)


class AliasEstimator(Estimator):
  """An estimator which aliases another estimator for a different op.

    Operations which have very similar or identical delay characteristics (for
    example, kSub and kAdd) can be modeled using an alias. For example, the
    estimator for kSub could be an AliasEstimator which refers to kAdd.
  """

  def __init__(self, op, aliased_op: str):
    super(AliasEstimator, self).__init__(op)
    self.aliased_op = aliased_op

  def cpp_delay_code(self, node_identifier: str) -> str:
    return 'return {}Delay({});'.format(
        self.aliased_op.lstrip('k'), node_identifier)

  def operation_delay(self, operation: delay_model_pb2.Operation) -> int:
    raise NotImplementedError


def delay_factor_description(factor: delay_model_pb2.DelayFactor) -> str:
  """Returns a brief description of a delay factor."""
  e = delay_model_pb2.DelayFactor.Source
  return {
      e.RESULT_BIT_COUNT:
          'Result bit count',
      e.OPERAND_BIT_COUNT:
          'Operand %d bit count' % factor.operand_number,
      e.OPERAND_COUNT:
          'Operand count',
      e.OPERAND_ELEMENT_COUNT:
          'Operand %d element count' % factor.operand_number,
      e.OPERAND_ELEMENT_BIT_COUNT:
          'Operand %d element bit count' % factor.operand_number
  }[factor.source]


def delay_expression_description(exp: delay_model_pb2.DelayExpression) -> str:
  """Returns a brief description of a delay expression."""
  if exp.HasField('bin_op'):
    lhs = delay_expression_description(exp.lhs_expression)
    rhs = delay_expression_description(exp.rhs_expression)
    e = delay_model_pb2.DelayExpression.BinaryOperation
    if exp.bin_op == e.ADD:
      return '({} + {})'.format(lhs, rhs)
    elif exp.bin_op == e.DIVIDE:
      return '({lhs} / ({rhs} < 1.0 ? 1.0 : {rhs})'.format(lhs=lhs, rhs=rhs)
    elif exp.bin_op == e.MAX:
      return 'max({}, {})'.format(lhs, rhs)
    elif exp.bin_op == e.MIN:
      return 'min({}, {})'.format(lhs, rhs)
    elif exp.bin_op == e.MULTIPLY:
      return '({} * {})'.format(lhs, rhs)
    elif exp.bin_op == e.POWER:
      return 'pow({}, {})'.format(lhs, rhs)
    else:
      assert exp.bin_op == e.SUB
      return '({} - {})'.format(lhs, rhs)
  elif exp.HasField('factor'):
    return delay_factor_description(exp.factor)
  else:
    assert exp.HasField('constant')
    return str(exp.constant)


def _operation_delay_factor(factor: delay_model_pb2.DelayFactor,
                            operation: delay_model_pb2.Operation) -> int:
  """Returns the value of a delay factor extracted from an operation."""
  e = delay_model_pb2.DelayFactor.Source
  if factor.source == e.RESULT_BIT_COUNT:
    return operation.bit_count
  elif factor.source == e.OPERAND_BIT_COUNT:
    operand = operation.operands[factor.operand_number]
    if operand.element_count > 0:
      return operand.bit_count * operand.element_count
    else:
      return operand.bit_count
  elif factor.source == e.OPERAND_COUNT:
    return len(operation.operands)
  elif factor.source == e.OPERAND_ELEMENT_COUNT:
    return operation.operands[factor.operand_number].element_count
  else:
    assert factor.source == e.OPERAND_ELEMENT_BIT_COUNT
    return operation.operands[factor.operand_number].bit_count


def _operation_delay_expression(expression: delay_model_pb2.DelayExpression,
                                operation: delay_model_pb2.Operation) -> int:
  """Returns the value of a delay expression extracted from an operation."""
  if expression.HasField('bin_op'):
    assert expression.HasField('lhs_expression')
    assert expression.HasField('rhs_expression')
    lhs_value = _operation_delay_expression(expression.lhs_expression,
                                            operation)
    rhs_value = _operation_delay_expression(expression.rhs_expression,
                                            operation)
    e = delay_model_pb2.DelayExpression.BinaryOperation
    return {
        e.ADD: lambda: lhs_value + rhs_value,
        e.DIVIDE: lambda: lhs_value / (1.0 if rhs_value < 1.0 else rhs_value),
        e.MAX: lambda: max(lhs_value, rhs_value),
        e.MIN: lambda: min(lhs_value, rhs_value),
        e.MULTIPLY: lambda: lhs_value * rhs_value,
        e.POWER: lambda: lhs_value**rhs_value,
        e.SUB: lambda: lhs_value - rhs_value,
    }[expression.bin_op]()

  if expression.HasField('factor'):
    return _operation_delay_factor(expression.factor, operation)

  assert expression.HasField('constant')
  return expression.constant


def _delay_factor_cpp_expression(factor: delay_model_pb2.DelayFactor,
                                 node_identifier: str) -> str:
  """Returns a C++ expression which computes a delay factor of an XLS Node*.

  Args:
    factor: The delay factor to extract.
    node_identifier: The identifier of the xls::Node* to extract the factor
      from.

  Returns:
    C++ expression computing the delay factor of a node. For example, if
    the delay factor is OPERAND_COUNT, the method might return:
    'node->operand_count()'.
  """
  e = delay_model_pb2.DelayFactor.Source
  return {
      e.RESULT_BIT_COUNT:
          lambda: '{}->GetType()->GetFlatBitCount()'.format(node_identifier),
      e.OPERAND_BIT_COUNT:
          lambda: '{}->operand({})->GetType()->GetFlatBitCount()'.format(
              node_identifier, factor.operand_number),
      e.OPERAND_COUNT:
          lambda: '{}->operand_count()'.format(node_identifier),
      e.OPERAND_ELEMENT_COUNT:
          lambda: '{}->operand({})->GetType()->AsArrayOrDie()->size()'.format(
              node_identifier, factor.operand_number),
      e.OPERAND_ELEMENT_BIT_COUNT:
          lambda:
          '{}->operand({})->GetType()->AsArrayOrDie()->element_type()->GetFlatBitCount()'
          .format(node_identifier, factor.operand_number),
  }[factor.source]()


def _delay_expression_cpp_expression(
    expression: delay_model_pb2.DelayExpression, node_identifier: str) -> str:
  """Returns a C++ expression which computes a delay expression of an XLS Node*.

  Args:
    expression: The delay expression to extract.
    node_identifier: The identifier of the xls::Node* to extract the factor
      from.

  Returns:
    C++ expression computing the delay expression of a node.
  """
  if expression.HasField('bin_op'):
    assert expression.HasField('lhs_expression')
    assert expression.HasField('rhs_expression')
    lhs_value = _delay_expression_cpp_expression(expression.lhs_expression,
                                                 node_identifier)
    rhs_value = _delay_expression_cpp_expression(expression.rhs_expression,
                                                 node_identifier)
    e = delay_model_pb2.DelayExpression.BinaryOperation
    return {
        e.ADD:
            lambda: '({} + {})'.format(lhs_value, rhs_value),
        e.DIVIDE:
            lambda: '({lhs} / ({rhs} < 1.0 ? 1.0 : {rhs}))'.format(
                lhs=lhs_value, rhs=rhs_value),
        e.MAX:
            lambda: 'std::max({}, {})'.format(lhs_value, rhs_value),
        e.MIN:
            lambda: 'std::min({}, {})'.format(lhs_value, rhs_value),
        e.MULTIPLY:
            lambda: '({} * {})'.format(lhs_value, rhs_value),
        e.POWER:
            lambda: 'pow({}, {})'.format(lhs_value, rhs_value),
        e.SUB:
            lambda: '({} - {})'.format(lhs_value, rhs_value),
    }[expression.bin_op]()

  if expression.HasField('factor'):
    return 'static_cast<float>({})'.format(
        _delay_factor_cpp_expression(expression.factor, node_identifier))

  assert expression.HasField('constant')
  return 'static_cast<float>({})'.format(expression.constant)


class RegressionEstimator(Estimator):
  """An estimator which uses curve fitting of measured data points.

  The curve has the form:

    delay_est = P_0 + P_1 * factor_0 + P_2 * log2(factor_0) +
                      P_3 * factor_1 + P_4 * log2(factor_1) +
                      ...

  Where P_i are learned parameters and factor_i are the delay expressions
  extracted from the operation (for example, operand count or result bit
  count or some mathemtical combination thereof). The model supports an
  arbitrary number of expressions.

  Attributes:
    delay_expressions: The expressions used in curve fitting.
    data_points: Delay measurements used by the model as DataPoint protos.
    raw_data_points: Delay measurements stored in RawDataPoint structures. The
      .delay_factors list contains the delay expressions, and the .delay_ps
      field is the measured delay.
    delay_function: The curve-fitted function which computes the estimated delay
      given the expressions as floats.
    params: The list of learned parameters.
    num_cross_validation_folds: The number of folds to use for cross validation.
    max_data_point_error: The maximum allowable absolute error for any single
      data point.
    max_fold_geomean_error: The maximum allowable geomean absolute error over
      all data points in a given test set.
  """

  def __init__(self,
               op,
               delay_expressions: Sequence[delay_model_pb2.DelayExpression],
               data_points: Sequence[delay_model_pb2.DataPoint],
               num_cross_validation_folds: int = 5,
               max_data_point_error: float = np.inf,
               max_fold_geomean_error: float = np.inf):
    super(RegressionEstimator, self).__init__(op)
    self.delay_expressions = list(delay_expressions)
    self.data_points = list(data_points)

    # Compute the raw data points for curve fitting. Each raw data point is a
    # tuple of numbers representing the delay expressions and the delay. For
    # example: (expression_0, expression_1, delay).
    self.raw_data_points = []
    for dp in self.data_points:
      self.raw_data_points.append(
          RawDataPoint(
              delay_factors=[
                  _operation_delay_expression(e, dp.operation)
                  for e in self.delay_expressions
              ],
              delay_ps=(dp.delay - dp.delay_offset)
          )
      )

    self._k_fold_cross_validation(self.raw_data_points,
                                  num_cross_validation_folds,
                                  max_data_point_error, max_fold_geomean_error)
    self.delay_function, self.params = self._fit_curve(self.raw_data_points)

  @staticmethod
  def generate_k_fold_cross_validation_train_and_test_sets(
      raw_data_points: Sequence[RawDataPoint],
      num_cross_validation_folds: int):
    """Yields training and testing datasets for cross validation.

    Args:
      raw_data_points: The sequence of data points.
      num_cross_validation_folds: Number of cross-validation folds.

    Yields:
      Yields training and testing datasets for cross
      validation. 'num_cross_validation_folds' number of training and testing
      datasets for use in cross validation.
    """

    # Separate data into num_cross_validation_folds sets
    random.seed(0)
    randomized_data_points = random.sample(raw_data_points,
                                           len(raw_data_points))
    folds = []
    for fold_idx in range(num_cross_validation_folds):
      folds.append([
          dp for idx, dp in enumerate(randomized_data_points)
          if idx % num_cross_validation_folds == fold_idx
      ])

    # Generate train  and test data points.
    for test_fold_idx in range(num_cross_validation_folds):
      training_dps = []
      for fold_idx, fold_dps in enumerate(folds):
        if fold_idx == test_fold_idx:
          continue
        training_dps.extend(fold_dps)
      yield training_dps, folds[test_fold_idx]

  def _k_fold_cross_validation(self, raw_data_points: Sequence[RawDataPoint],
                               num_cross_validation_folds: int,
                               max_data_point_error: float,
                               max_fold_geomean_error: float):
    """Perfroms k-fold cross validation to verify the model.

    An exception is raised if the model does not pass cross validation.  Note
    that this function modifies self.delay_function to perform regression on
    partial data sets.

    Args:
      raw_data_points: A sequence of RawDataPoints, where each is a single
        measurement point.  Independent variables are in the .delay_factors
        field, and the dependent variable is in the .delay_ps field.
      num_cross_validation_folds: The number of folds to use for cross
        validation.
      max_data_point_error: The maximum allowable absolute error for any single
        data point.
      max_fold_geomean_error: The maximum allowable geomean absolute error over
        all data points in a given test set.

    Raises:
      Error: Raised if the model does not pass cross validation.  Note
        that this function modifies self.delay_function to perform regression on
        partial data sets.

    """
    if max_data_point_error == np.inf and max_fold_geomean_error == np.inf:
      return
    if num_cross_validation_folds > len(raw_data_points):
      raise Error('{}: Too few data points to cross validate: '
                  '{} data points, {} folds'.format(self.op,
                                                    len(raw_data_points),
                                                    num_cross_validation_folds))

    # Perform validation for each training and testing set.
    for (
        training_dps,
        testing_dps
    ) in RegressionEstimator.generate_k_fold_cross_validation_train_and_test_sets(
        raw_data_points, num_cross_validation_folds=num_cross_validation_folds
    ):

      # Train.
      self.delay_function, self.params = self._fit_curve(training_dps)

      # Test.
      error_product = 1.0
      for dp in testing_dps:
        xdata = dp.delay_factors
        ydata = dp.delay_ps
        predicted_delay = self.raw_delay(xdata)
        abs_dp_error = abs((predicted_delay - ydata) / ydata)
        error_product = error_product * abs_dp_error
        if abs_dp_error > max_data_point_error:
          raise Error('{}: Regression model failed k-fold cross validation for '
                      'data point {} with absolute error {} > max {}'.format(
                          self.op, dp, abs_dp_error, max_data_point_error))
      geomean_error = error_product**(1.0 / len(testing_dps))
      if geomean_error > max_fold_geomean_error:
        raise Error('{}: Regression model failed k-fold cross validation for '
                    'test set with geomean error {} > max {}'.format(
                        self.op, geomean_error, max_fold_geomean_error))

  def _fit_curve(
      self, raw_data_points: Sequence[RawDataPoint]
  ) -> Tuple[Callable[[Sequence[float]], float], np.ndarray]:
    """Fits a curve to the given data points.

    Args:
      raw_data_points: A sequence of RawDataPoints, where each is a single
        measurement point.  Independent variables are in the .delay_factors
        field, and the dependent variable is in the .delay_ps field.

    Returns:
      A tuple containing the fitted function and the sequence of learned
      parameters.
    """
    # Split the raw data points into independent (xdata) and dependent variables
    # (ydata).
    raw_xdata = np.array(
        [pt.delay_factors for pt in raw_data_points], dtype=np.float64
    )
    ydata = np.transpose([pt.delay_ps for pt in raw_data_points])

    # Construct our augmented "independent" variables in a matrix:
    # xdata = [1, x0, log2(x0), x1, log2(x1), ...]
    def augment_xdata(x_arr: np.ndarray) -> np.ndarray:
      x_augmented = np.ones(
          (x_arr.shape[0], 1 + 2 * x_arr.shape[1]), dtype=np.float64
      )
      x_augmented[::, 1::2] = x_arr
      x_augmented[::, 2::2] = np.log2(np.maximum(1.0, x_arr))
      return x_augmented
    xdata = augment_xdata(raw_xdata)

    # Now, the least-squares solution to the equation xdata @ p = ydata is
    # exactly the set of parameters for our model! EXCEPT: we want to make sure
    # none of the weights are negative, since we expect all terms to have net
    # positive contribution. This helps make sure extrapolations are reasonable.
    params = opt.nnls(xdata, ydata)[0]

    def delay_f(x) -> float:
      x_augmented = augment_xdata(np.array([x], dtype=np.float64))
      return np.dot(x_augmented, params)[0]

    return delay_f, params.flatten()

  def operation_delay(self, operation: delay_model_pb2.Operation) -> int:
    expressions = tuple(
        _operation_delay_expression(e, operation)
        for e in self.delay_expressions)
    return int(self.delay_function(expressions))

  def raw_delay(self, xargs: Sequence[float]) -> float:
    """Returns the delay with delay expressions passed in as floats."""
    return self.delay_function(xargs)

  def cpp_delay_code(self, node_identifier: str) -> str:
    terms = [repr(self.params[0])]
    for i, expression in enumerate(self.delay_expressions):
      e_str = _delay_expression_cpp_expression(expression, node_identifier)
      terms.append('{!r} * {}'.format(self.params[2 * i + 1], e_str))
      terms.append('{w!r} * std::log2({e} < 1.0 ? 1.0 : {e})'.format(
          w=self.params[2 * i + 2], e=e_str))
    return 'return std::round({});'.format(' + '.join(terms))


class BoundingBoxEstimator(Estimator):
  """Bounding box estimator."""

  def __init__(self, op, factors: Sequence[delay_model_pb2.DelayFactor],
               data_points: Sequence[delay_model_pb2.DataPoint]):
    super(BoundingBoxEstimator, self).__init__(op)
    self.delay_factors = factors
    self.data_points = list(data_points)
    self.raw_data_points = []
    for dp in self.data_points:
      self.raw_data_points.append(
          RawDataPoint(
              delay_factors=[
                  _operation_delay_factor(e, dp.operation)
                  for e in self.delay_factors
              ],
              delay_ps=(dp.delay - dp.delay_offset)
          )
      )

  def cpp_delay_code(self, node_identifier: str) -> str:
    lines = []
    for raw_data_point in self.raw_data_points:
      test_expr_terms = []
      for i, x_value in enumerate(raw_data_point.delay_factors):
        test_expr_terms.append('%s <= %d' % (_delay_factor_cpp_expression(
            self.delay_factors[i], node_identifier), x_value))
      lines.append('if (%s) { return %d; }' %
                   (' && '.join(test_expr_terms), raw_data_point.delay_ps))
    lines.append(
        'return absl::UnimplementedError('
        '"Unhandled node for delay estimation: " '
        '+ {}->ToStringWithOperandTypes());'.format(node_identifier))
    return '\n'.join(lines)

  def operation_delay(self, operation: delay_model_pb2.Operation) -> int:
    xargs = tuple(
        _operation_delay_factor(f, operation) for f in self.delay_factors)
    return int(self.raw_delay(xargs))

  def raw_delay(self, xargs):
    """Returns the delay with delay factors passed in as floats."""
    for raw_data_point in self.raw_data_points:
      x_values = raw_data_point.delay_factors
      if all(a <= b for (a, b) in zip(xargs, x_values)):
        return raw_data_point.delay_ps
    raise Error('Operation outside bounding box')


class LogicalEffortEstimator(Estimator):
  """A delay estimator which uses logical effort computation.

  Attributes:
    tau_in_ps: The delay of a single inverter in ps.
  """

  def __init__(self, op, tau_in_ps: int):
    super(LogicalEffortEstimator, self).__init__(op)
    self.tau_in_ps = tau_in_ps

  def operation_delay(self, operation: delay_model_pb2.Operation) -> int:
    raise NotImplementedError

  def cpp_delay_code(self, node_identifier: str) -> str:
    lines = []
    lines.append(
        'absl::StatusOr<int64_t> delay_in_ps = '
        'DelayEstimator::GetLogicalEffortDelayInPs({}, {});'
        .format(node_identifier, self.tau_in_ps))
    lines.append('if (delay_in_ps.ok()) {')
    lines.append('  return delay_in_ps.value();')
    lines.append('}')
    lines.append('return delay_in_ps.status();')
    return '\n'.join(lines)


def _estimator_from_proto(op: str, proto: delay_model_pb2.Estimator,
                          data_points: Sequence[delay_model_pb2.DataPoint]):
  """Create an Estimator from a proto."""
  if proto.HasField('fixed'):
    assert not data_points
    return FixedEstimator(op, proto.fixed)
  if proto.HasField('alias_op'):
    assert not data_points
    return AliasEstimator(op, proto.alias_op)
  if proto.HasField('regression'):
    assert data_points
    keyword_dict = dict()
    if proto.regression.HasField('kfold_validator'):
      optional_args = [
          'num_cross_validation_folds', 'max_data_point_error',
          'max_fold_geomean_error'
      ]
      for arg in optional_args:
        if proto.regression.kfold_validator.HasField(arg):
          keyword_dict[arg] = getattr(proto.regression.kfold_validator, arg)
    return RegressionEstimator(op, proto.regression.expressions, data_points,
                               **keyword_dict)
  if proto.HasField('bounding_box'):
    assert data_points
    return BoundingBoxEstimator(op, proto.bounding_box.factors, data_points)
  assert proto.HasField('logical_effort')
  assert not data_points
  return LogicalEffortEstimator(op, proto.logical_effort.tau_in_ps)


class OpModel:
  """Delay model for a single XLS op (e.g., kAdd).

  This abstraction mirrors the OpModel proto message in delay_model.proto.

  Attributes:
    op: The op for this model (e.g., 'kAdd').
    specializations: A map from SpecializationKind to Estimator which contains
      any specializations of the delay model of the op.
    estimator: The non-specialized Estimator to use for this op in the general
      case.
  """

  def __init__(self, proto: delay_model_pb2.OpModel,
               data_points: Sequence[delay_model_pb2.DataPoint]):
    self.op = proto.op
    data_points = list(data_points)
    # Build a separate estimator for each specialization, if any.
    self.specializations = {}
    for specialization in proto.specializations:
      # pylint: disable=cell-var-from-loop
      pred = lambda dp: dp.operation.specialization == specialization.kind
      # Filter out the data points which correspond to the specialization.
      special_data_points = [dp for dp in data_points if pred(dp)]
      data_points = [dp for dp in data_points if not pred(dp)]
      self.specializations[specialization.kind] = _estimator_from_proto(
          self.op, specialization.estimator, special_data_points)
    self.estimator = _estimator_from_proto(self.op, proto.estimator,
                                           data_points)

  def cpp_delay_function(self) -> str:
    """Return a C++ function which computes delay for an operation."""
    lines = []
    lines.append('absl::StatusOr<int64_t> %s(Node* node) {' %
                 self.cpp_delay_function_name())
    for kind, estimator in self.specializations.items():
      if kind == delay_model_pb2.SpecializationKind.OPERANDS_IDENTICAL:
        cond = ('std::all_of(node->operands().begin(), node->operands().end(), '
                '[&](Node* n) { return n == node->operand(0); })')
      elif kind == delay_model_pb2.SpecializationKind.HAS_LITERAL_OPERAND:
        cond = ('std::any_of(node->operands().begin(), node->operands().end(), '
                '[](Node* n) { return n->Is<Literal>(); })')
      else:
        raise NotImplementedError
      lines.append('if (%s) {' % cond)
      lines.append(estimator.cpp_delay_code('node'))
      lines.append('}')
    lines.append(self.estimator.cpp_delay_code('node'))
    lines.append('}')
    return '\n'.join(lines)

  def cpp_delay_function_name(self) -> str:
    return self.op.lstrip('k') + 'Delay'

  def cpp_delay_function_declaration(self) -> str:
    return 'absl::StatusOr<int64_t> {}(Node* node);'.format(
        self.cpp_delay_function_name())


class DelayModel:
  """Delay model representing a particular hardware technology.

  Attributes:
    op_models: A map from xls::Op (e.g., 'kAdd') to the OpModel for that op.
  """

  def __init__(self, proto: delay_model_pb2.DelayModel):
    op_data_points = {}
    for data_point in proto.data_points:
      op = data_point.operation.op
      op_data_points[op] = op_data_points.get(op, []) + [data_point]

    self.op_models = {}
    for op_model in proto.op_models:
      self.op_models[op_model.op] = OpModel(op_model,
                                            op_data_points.get(op_model.op, ()))

  def ops(self) -> Sequence[str]:
    return sorted(self.op_models.keys())

  def op_model(self, op: str) -> OpModel:
    return self.op_models[op]
