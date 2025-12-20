#include <iconv.h>
#include <errno.h>

bool is_valid_utf8(const char* data, size_t len)
{
	iconv_t cd = iconv_open("UTF-8", "UTF-8");
	if (cd == (iconv_t)-1)
		return false;
	char *in = const_cast<char*>(data), out[4], *outp;
	size_t outl;
	while (len > 0)
	{
		outp = out;
		outl = 4;
		if (iconv(cd, &in, &len, &outp, &outl) == (size_t)-1 && errno != E2BIG)
		{
			iconv_close(cd);
			return false;
		}
	}
	iconv_close(cd);
	return true;
}
