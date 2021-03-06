#include <array>
#include <sstream>

#include "Log.h"
#include "GLLoader.h"
#include "FileUtil.h"
#include "Texture.h"
#include "EnumUtil.h"
#include "EntityUtil.h"

namespace fury
{
	Texture::Ptr Texture::Create(const std::string &name)
	{
		return std::make_shared<Texture>(name);
	}

	Texture::Ptr Texture::Get(int width, int height, TextureFormat format, TextureType type)
	{
		std::stringstream ss;
		ss << width << "*" << height << "*" << EnumUtil::TextureFormatToString(format) << 
			"*" << EnumUtil::TextureTypeToString(type);
		std::string name = ss.str();

		auto match = EntityUtil::Instance()->Get<Texture>(name);
		if (match == nullptr)
		{
			match = Texture::Create(name);
			match->CreateEmpty(width, height, format, type);
			EntityUtil::Instance()->Add(match);
		}

		return match;
	}

	Texture::Texture(const std::string &name)
		: Entity(name), m_BorderColor(0, 0, 0, 0)
	{
		m_TypeIndex = typeid(Texture);
		m_TypeUint = EnumUtil::TextureTypeToUnit(m_Type);
	}

	Texture::~Texture()
	{
		DeleteBuffer();
	}

	bool Texture::Load(const void* wrapper)
	{
		std::string str;

		if (!IsObject(wrapper)) return false;

		if (!LoadMemberValue(wrapper, "format", str))
		{
			FURYE << "Texture param 'format' not found!";
			return false;
		}
		auto format = EnumUtil::TextureFormatFromString(str);
		
		if (!LoadMemberValue(wrapper, "type", str))
			str = EnumUtil::TextureTypeToString(TextureType::TEXTURE_2D);
		auto type = EnumUtil::TextureTypeFromString(str);

		int width, height;
		if (!LoadMemberValue(wrapper, "width", width) || !LoadMemberValue(wrapper, "height", height))
		{
			FURYE << "Texture param 'width/height' not found!";
			return false;
		}
		
		auto filterMode = FilterMode::LINEAR;
		if (LoadMemberValue(wrapper, "filter", str))
			filterMode = EnumUtil::FilterModeFromString(str);

		auto wrapMode = WrapMode::REPEAT;
		if (LoadMemberValue(wrapper, "wrap", str))
			wrapMode = EnumUtil::WrapModeFromString(str);

		auto color = Color::Black;
		LoadMemberValue(wrapper, "borderColor", color);
		SetBorderColor(color);

		bool mipmap = false;
		LoadMemberValue(wrapper, "mipmap", mipmap);

		SetFilterMode(filterMode);
		SetWrapMode(wrapMode);

		CreateEmpty(width, height, format, type, mipmap);

		return true;
	}

	bool Texture::Save(void* wrapper)
	{
		StartObject(wrapper);

		SaveKey(wrapper, "name");
		SaveValue(wrapper, m_Name);
		SaveKey(wrapper, "format");
		SaveValue(wrapper, EnumUtil::TextureFormatToString(m_Format));
		SaveKey(wrapper, "type");
		SaveValue(wrapper, EnumUtil::TextureTypeToString(m_Type));
		SaveKey(wrapper, "filter");
		SaveValue(wrapper, EnumUtil::FilterModeToString(m_FilterMode));
		SaveKey(wrapper, "wrap");
		SaveValue(wrapper, EnumUtil::WrapModeToString(m_WrapMode));
		SaveKey(wrapper, "width");
		SaveValue(wrapper, m_Width);
		SaveKey(wrapper, "height");
		SaveValue(wrapper, m_Height);
		SaveKey(wrapper, "borderColor");
		SaveValue(wrapper, m_BorderColor);
		SaveKey(wrapper, "mipmap");
		SaveValue(wrapper, m_Mipmap);

		EndObject(wrapper);

		return true;
	}

	void Texture::CreateFromImage(std::string filePath, bool mipMap)
	{
		DeleteBuffer();

		int channels;
		std::vector<unsigned char> pixels;
		if (FileUtil::LoadImage(filePath, pixels, m_Width, m_Height, channels))
		{
			unsigned int internalFormat, imageFormat;

			switch (channels)
			{
			case 1:
				m_Format = TextureFormat::R8;
				internalFormat = GL_R8;
				imageFormat = GL_RED;
				break;
			case 2:
				m_Format = TextureFormat::RG8;
				internalFormat = GL_RG8;
				imageFormat = GL_RG;
				break;
			case 3:
				m_Format = TextureFormat::RGB8;
				internalFormat = GL_RGB8;
				imageFormat = GL_RGB;
				break;
			case 4: 
				m_Format = TextureFormat::RGBA8;
				internalFormat = GL_RGBA8;
				imageFormat = GL_RGBA;
				break;
			default:
				m_Format = TextureFormat::UNKNOW;
				FURYW << channels << " channel image not supported!";
				return;
			}

			m_Mipmap = mipMap;
			m_FilePath = filePath;
			m_Dirty = false;

			glGenTextures(1, &m_ID);
			glBindTexture(m_TypeUint, m_ID);

			glTexStorage2D(m_TypeUint, m_Mipmap ? FURY_MIPMAP_LEVEL : 1, internalFormat, m_Width, m_Height);
			glTexSubImage2D(m_TypeUint, 0, 0, 0, m_Width, m_Height, imageFormat, GL_UNSIGNED_BYTE, &pixels[0]);
			
			unsigned int filterMode = EnumUtil::FilterModeToUint(m_FilterMode);
			unsigned int wrapMode = EnumUtil::WrapModeToUint(m_WrapMode);

			glTexParameteri(m_TypeUint, GL_TEXTURE_MIN_FILTER, filterMode);
			glTexParameteri(m_TypeUint, GL_TEXTURE_MAG_FILTER, filterMode);
			glTexParameteri(m_TypeUint, GL_TEXTURE_WRAP_S, wrapMode);
			glTexParameteri(m_TypeUint, GL_TEXTURE_WRAP_T, wrapMode);
			glTexParameteri(m_TypeUint, GL_TEXTURE_WRAP_R, wrapMode);

			float color[] = { m_BorderColor.r, m_BorderColor.g, m_BorderColor.b, m_BorderColor.a };
			glTexParameterfv(m_TypeUint, GL_TEXTURE_BORDER_COLOR, color);

			if (m_Mipmap)
				glGenerateMipmap(m_TypeUint);

			glBindTexture(m_TypeUint, 0);

			FURYD << m_Name << " [" << m_Width << " x " << m_Height <<  " x " << EnumUtil::TextureTypeToString(m_Type) << "]";
		}
	}

	void Texture::CreateEmpty(int width, int height, TextureFormat format, TextureType type, bool mipMap)
	{
		DeleteBuffer();

		if (format == TextureFormat::UNKNOW)
			return;
		
		m_Mipmap = mipMap;
		m_Format = format;
		m_Dirty = false;
		m_Width = width;
		m_Height = height;
		
		m_Type = type;
		m_TypeUint = EnumUtil::TextureTypeToUnit(m_Type);

		unsigned int internalFormat = EnumUtil::TextureFormatToUint(format).second;

		glGenTextures(1, &m_ID);
		glBindTexture(m_TypeUint, m_ID);
		glTexStorage2D(m_TypeUint, m_Mipmap ? FURY_MIPMAP_LEVEL : 1, internalFormat, width, height);

		unsigned int filterMode = EnumUtil::FilterModeToUint(m_FilterMode);
		unsigned int wrapMode = EnumUtil::WrapModeToUint(m_WrapMode);

		glTexParameteri(m_TypeUint, GL_TEXTURE_MIN_FILTER, filterMode);
		glTexParameteri(m_TypeUint, GL_TEXTURE_MAG_FILTER, filterMode);
		glTexParameteri(m_TypeUint, GL_TEXTURE_WRAP_S, wrapMode);
		glTexParameteri(m_TypeUint, GL_TEXTURE_WRAP_T, wrapMode);
		glTexParameteri(m_TypeUint, GL_TEXTURE_WRAP_R, wrapMode);
		
		float color[] = { m_BorderColor.r, m_BorderColor.g, m_BorderColor.b, m_BorderColor.a };
		glTexParameterfv(m_TypeUint, GL_TEXTURE_BORDER_COLOR, color);

		if (m_Mipmap)
			glGenerateMipmap(m_TypeUint);

		glBindTexture(m_TypeUint, 0);

		FURYD << m_Name << " [" << m_Width << " x " << m_Height << " x " << EnumUtil::TextureTypeToString(m_Type) << "]";
	}

	void Texture::Update(const void* pixels)
	{
		if (m_ID == 0)
		{
			FURYW << "Texture buffer not created yet!";
			return;
		}

		glBindTexture(m_TypeUint, m_ID);
		glTexSubImage2D(m_TypeUint, 0, 0, 0, m_Width, m_Height, EnumUtil::TextureFormatToUint(m_Format).second, GL_UNSIGNED_BYTE, pixels);

		if (m_Mipmap)
			glGenerateMipmap(m_TypeUint);

		glBindTexture(m_TypeUint, 0);
	}

	void Texture::DeleteBuffer()
	{
		m_Dirty = true;

		if (m_ID != 0)
		{
			glDeleteTextures(1, &m_ID);
			m_ID = 0;
			m_Width = m_Height = 0;
			m_Format = TextureFormat::UNKNOW;
			m_FilePath = "";
		}
	}

	TextureFormat Texture::GetFormat() const
	{
		return m_Format;
	}

	TextureType Texture::GetType() const
	{
		return m_Type;
	}

	unsigned int Texture::GetTypeUint() const
	{
		return m_TypeUint;
	}

	FilterMode Texture::GetFilterMode() const
	{
		return m_FilterMode;
	}

	void Texture::SetFilterMode(FilterMode mode)
	{
		if (m_FilterMode != mode)
		{
			m_FilterMode = mode;
			if (m_ID != 0)
			{
				glBindTexture(m_TypeUint, m_ID);

				unsigned int filterMode = EnumUtil::FilterModeToUint(m_FilterMode);
				glTexParameteri(m_TypeUint, GL_TEXTURE_MIN_FILTER, filterMode);
				glTexParameteri(m_TypeUint, GL_TEXTURE_MAG_FILTER, filterMode);

				glBindTexture(m_TypeUint, 0);
			}
		}
	}

	WrapMode Texture::GetWrapMode() const
	{
		return m_WrapMode;
	}

	void Texture::SetWrapMode(WrapMode mode)
	{
		if (m_WrapMode != mode)
		{
			m_WrapMode = mode;
			if (m_ID != 0)
			{
				glBindTexture(m_TypeUint, m_ID);

				unsigned int wrapMode = EnumUtil::WrapModeToUint(m_WrapMode);
				glTexParameteri(m_TypeUint, GL_TEXTURE_WRAP_S, wrapMode);
				glTexParameteri(m_TypeUint, GL_TEXTURE_WRAP_T, wrapMode);
				glTexParameteri(m_TypeUint, GL_TEXTURE_WRAP_R, wrapMode);

				glBindTexture(m_TypeUint, 0);
			}
		}
	}

	Color Texture::GetBorderColor() const
	{
		return m_BorderColor;
	}

	void Texture::SetBorderColor(Color color)
	{
		if (m_BorderColor != color)
		{
			m_BorderColor = color;
			if (m_ID != 0)
			{
				glBindTexture(m_TypeUint, m_ID);

				float color[] = { m_BorderColor.r, m_BorderColor.g, m_BorderColor.b, m_BorderColor.a };
				glTexParameterfv(m_TypeUint, GL_TEXTURE_BORDER_COLOR, color);

				glBindTexture(m_TypeUint, 0);
			}
		}
	}

	void Texture::GenerateMipMap()
	{
		if (m_ID == 0)
			return;

		m_Mipmap = true;

		glBindTexture(m_TypeUint, m_ID);
		glGenerateMipmap(m_TypeUint);
	}

	bool Texture::GetMipmap() const
	{
		return m_Mipmap;
	}

	int Texture::GetWidth() const
	{
		return m_Width;
	}

	int Texture::GetHeight() const
	{
		return m_Height;
	}

	unsigned int Texture::GetID() const
	{
		return m_ID;
	}

	std::string Texture::GetFilePath() const
	{
		return m_FilePath;
	}
}