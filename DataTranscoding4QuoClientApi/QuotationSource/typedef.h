
#ifndef _TYPEDEF_H_
#define	_TYPEDEF_H_

	#ifndef	LINUXCODE

	typedef void					df_void_t;
	typedef bool					df_bool_t;
	typedef char					df_char_t;
	typedef unsigned char			df_uchar_t;
	typedef short					df_short_t;
	typedef unsigned short			df_ushort_t;
	typedef int						df_int_t;
	typedef unsigned int			df_uint_t;
	typedef long					df_long_t;
	typedef unsigned long			df_ulong_t;
	typedef __int64					df_llong_t;
	typedef unsigned __int64		df_ullong_t;
	typedef float					df_float_t;
	typedef double					df_double_t;

	typedef char					df_s8_t;
	typedef unsigned char			df_u8_t;
	typedef short					df_s16_t;
	typedef unsigned short			df_u16_t;
	typedef int						df_s32_t;
	typedef unsigned int			df_u32_t;
	typedef __int64					df_s64_t;
	typedef unsigned __int64		df_u64_t;

	#else

	typedef void					df_void_t;
	typedef bool					df_bool_t;
	typedef char					df_char_t;
	typedef unsigned char			df_uchar_t;
	typedef short					df_short_t;
	typedef unsigned short			df_ushort_t;
	typedef int						df_int_t;
	typedef unsigned int			df_uint_t;
	typedef long					df_long_t;
	typedef unsigned long			df_ulong_t;
	typedef long long				df_llong_t;
	typedef unsigned long long		df_ullong_t;
	typedef float					df_float_t;
	typedef double					df_double_t;

	typedef char					df_s8_t;
	typedef unsigned char			df_u8_t;
	typedef short					df_s16_t;
	typedef unsigned short			df_u16_t;
	typedef int						df_s32_t;
	typedef unsigned int			df_u32_t;
	typedef long long				df_s64_t;
	typedef unsigned long long		df_u64_t;

	#endif		// LINUXCODE

#endif		// _TYPEDEF_H_
