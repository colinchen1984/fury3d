#ifndef _FURY_FILEUTIL_H_
#define _FURY_FILEUTIL_H_

#include <string>
#include <vector>

#include "EnumUtil.h"

#undef LoadString
#undef LoadImage

namespace fury
{
	class Pipeline;

	class Serializable;

	class FURY_API FileUtil final
	{
	private:

		static std::string m_AbsPath;

	public:

		static std::string GetAbsPath();

		static std::string GetAbsPath(const std::string &source, bool toForwardSlash = false);

		static bool FileExist(const std::string &path);

		// image, text file io

		static bool LoadString(const std::string &path, std::string &output);

		static bool LoadImage(const std::string &path, std::vector<unsigned char> &output, int &width, int &height, int &channels);

		// serializable obj io

		static bool LoadFromFile(const std::shared_ptr<Serializable> &source, const std::string &filePath);

		static bool SaveToFile(const std::shared_ptr<Serializable> &source, const std::string &filePath);
	};
}

#endif // _FURY_FILEUTIL_H_