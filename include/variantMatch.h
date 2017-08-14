#pragma once
#include <boost/variant.hpp>

namespace helpers
{
  // deduce return type of operator()
  template<typename T>
  struct return_type;

  template<typename Res, typename... Args>
  struct return_type<Res(*)(Args...)>
  {
    using type = Res;
  };

  template<typename T, typename Res, typename... Args>
  struct return_type<Res(T::*)(Args...)>
  {
    using type = Res;
  };

  template<typename T, typename Res, typename... Args>
  struct return_type<Res(T::*)(Args...) const>
  {
    using type = Res;
  };

  template<typename FunctionObject>
  struct function_object_return_type : return_type<decltype(&FunctionObject::operator())>
  {};

  // https://www.youtube.com/watch?v=mqei4JJRQ7s
  // make an overload of multiple function objects
  template<typename Res, typename Func, typename... Funcs>
  struct overload_set : Func, overload_set<Res, Funcs...>
  {
    using result_type = Res;

    using Func::operator();
    using overload_set<Res, Funcs...>::operator();

    template<typename FuncFwd, typename... FuncFwds>
    overload_set(FuncFwd&& f, FuncFwds... fs)
      : Func(std::forward<FuncFwd>(f)),
      overload_set<Res, Funcs...>(std::forward<FuncFwds>(fs)...)
    {}
  };

  template<typename Res, typename Func>
  struct overload_set<Res, Func> : Func
  {
    using result_type = Res;

    using Func::operator();

    template<typename FuncFwd>
    overload_set(FuncFwd&& f)
      : Func(std::forward<FuncFwd>(f))
    {}
  };

  template<typename Res, typename... Funcs>
  overload_set<Res, typename std::remove_reference<Funcs>::type...> overload(Funcs&&... fs)
  {
    using os = overload_set<Res, typename std::remove_reference<Funcs>::type...>;
    return os(std::forward<Funcs>(fs)...);
  }

  // a function object which calls one of the given overloads depending on the variants value
  template<typename Variant>
  struct matcher
  {
    matcher(Variant& variant)
      : m_variant(variant)
    {}


    template<typename Func, typename... Funcs>
    typename overload_set<typename function_object_return_type<Func>::type, typename std::remove_reference<Func>::type, typename std::remove_reference<Funcs>::type...>::result_type
      operator()(Func&& f, Funcs&&... fs) const
    {
      using Res = typename function_object_return_type<Func>::type;
      auto visitor = overload<Res>(std::forward<Func>(f), std::forward<Funcs>(fs)...);
      return boost::apply_visitor(visitor, m_variant);
    }

    Variant& m_variant;
  };

  template<typename Variant>
  auto match(Variant&& variant)->matcher<Variant>
  {
    return matcher<Variant>{std::forward<Variant>(variant)};
  }
}