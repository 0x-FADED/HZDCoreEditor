#pragma once

#include <format>

namespace RTTI_fmt
{
	template <std::output_iterator<const char&> _OutputIt, class... _Types>
	inline std::format_to_n_result<_OutputIt> fmt( _OutputIt _Out, const std::iter_difference_t<_OutputIt> _Max, const std::string_view _Fmt, _Types&&... _Args) 
	{
		std::_Fmt_iterator_buffer<_OutputIt, char, std::_Fmt_fixed_buffer_traits> _Buf(std::move(_Out), _Max);
		std:: vformat_to(std::_Fmt_it{ _Buf }, _Fmt, _STD make_format_args(_Args...));
		return { .out = _Buf._Out(), .size = _Buf._Count() };
	}
}