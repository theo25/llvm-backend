#include <kllvm/ast/AST.h>
#include <kllvm/parser/KOREParser.h>

#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <sstream>
#include <tuple>

#include "runtime.h"

namespace py = pybind11;

using namespace kllvm;
using namespace kllvm::parser;

namespace detail {

template <typename T>
struct type_identity {
  using type = T;
};

/**
 * Adapt an AST node's print method to return a string for use with Python's
 * __repr__ method.
 */
template <typename T, typename... Args>
struct print_repr_adapter_st {
  print_repr_adapter_st(type_identity<T>, Args &&...args)
      : args_(std::forward<Args>(args)...) { }

  std::string operator()(T &node) {
    auto ss = std::stringstream{};

    std::apply(
        [&](auto &&...args) { return node.print(args...); },
        std::tuple_cat(
            std::tuple{std::ref(ss)}, std::forward<decltype(args_)>(args_)));

    return ss.str();
  }

private:
  std::tuple<Args...> args_;
};

template <typename T, typename... Args>
print_repr_adapter_st(type_identity<T>, Args &&...)
    -> print_repr_adapter_st<T, Args...>;

} // namespace detail

template <typename T, typename... Args>
auto print_repr_adapter(Args &&...args) {
  return ::detail::print_repr_adapter_st(
      ::detail::type_identity<T>{}, std::forward<Args>(args)...);
}

void bind_ast(py::module_ &m) {
  auto ast = m.def_submodule("ast", "K LLVM backend KORE AST");

  /* Data Types */

  py::enum_<SortCategory>(ast, "SortCategory")
      .value("Uncomputed", SortCategory::Uncomputed)
      .value("Map", SortCategory::Map)
      .value("List", SortCategory::List)
      .value("Set", SortCategory::Set)
      .value("Int", SortCategory::Int)
      .value("Float", SortCategory::Float)
      .value("StringBuffer", SortCategory::StringBuffer)
      .value("Bool", SortCategory::Bool)
      .value("Symbol", SortCategory::Symbol)
      .value("Variable", SortCategory::Variable)
      .value("MInt", SortCategory::MInt);

  py::class_<ValueType>(ast, "ValueType")
      .def(py::init([](SortCategory cat) {
        return ValueType{cat, 0};
      }))
      .def(py::init([](SortCategory cat, uint64_t bits) {
        return ValueType{cat, bits};
      }));

  /* Sorts */

  auto sort_base
      = py::class_<KORESort, std::shared_ptr<KORESort>>(ast, "Sort")
            .def_property_readonly("is_concrete", &KORESort::isConcrete)
            .def("substitute", &KORESort::substitute)
            .def("__repr__", print_repr_adapter<KORESort>())
            .def(
                "__hash__",
                [](KORESort const &sort) { return HashSort{}(sort); })
            .def(py::self == py::self)
            .def(py::self != py::self);

  py::class_<KORESortVariable, std::shared_ptr<KORESortVariable>>(
      ast, "SortVariable", sort_base)
      .def(py::init(&KORESortVariable::Create));

  py::class_<KORECompositeSort, std::shared_ptr<KORECompositeSort>>(
      ast, "CompositeSort", sort_base)
      .def(
          py::init(&KORECompositeSort::Create), py::arg("name"),
          py::arg("cat") = ValueType{SortCategory::Uncomputed, 0})
      .def("add_argument", &KORECompositeSort::addArgument)
      .def_property_readonly("name", &KORECompositeSort::getName);

  /* Symbols */

  py::class_<KORESymbol>(ast, "Symbol")
      .def(py::init(&KORESymbol::Create))
      .def("__repr__", print_repr_adapter<KORESymbol>())
      .def("to_str", print_repr_adapter<KORESymbol>())
      .def("to_str_params", print_repr_adapter<KORESymbol>(0, false))
      .def("add_argument", &KORESymbol::addArgument)
      .def("add_formal_argument", &KORESymbol::addFormalArgument)
      .def("add_sort", &KORESymbol::addSort);

  py::class_<KOREVariable>(ast, "Variable")
      .def(py::init(&KOREVariable::Create))
      .def("__repr__", print_repr_adapter<KOREVariable>())
      .def_property_readonly("name", &KOREVariable::getName);
}

PYBIND11_MODULE(_kllvm, m) {
  bind_ast(m);
}
