#include "UI.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "glad.h"

#include <algorithm>

#include "../Platform/Platform.hpp"

#include "imgui/imgui_internal.h"
#include "../ViewFunctions.hpp"

#include "../../Libs/SOIL2/SOIL2.h"

static_assert(sizeof(GLuint) <= sizeof(ImTextureID)); //We must not lose data when passing the image to ImGui

//moved here from ViewFunctions as it needs static variables
void Utils::ImGuiDragWithLastElement()
{
	static float scrollY = 0;
	static ImGuiID PrevItem = 0;
	const auto ScrollItem = GImGui->CurrentWindow->DC.LastItemId;
	if (ImGui::IsItemActive()) // Is the scrolling item active ?
	{
		if (!PrevItem) //If we're not scrolling, begin.
		{
			PrevItem = ScrollItem;
			scrollY = ImGui::GetScrollY();
		}
		if (PrevItem == ScrollItem) //Calculate the scrolling
		{			
			ImVec2 drag = ImGui::GetMouseDragDelta(0);
			ImGui::SetScrollY(scrollY - drag.y);
		}
	}	
	else if (PrevItem == ScrollItem) //we were scrolling but now we stopped
	{
		scrollY = 0;
		PrevItem = 0;
	}
}

size_t RenderImage::LeakCount = 0;

RenderImage::RenderImage(const std::vector<u8>& data)
{
	auto tex = SOIL_load_OGL_texture_from_memory(data.data(), data.size(), 4, 0, 0);

	if (tex <= 0)
	{
		LOGf("Failed to load image, SOIL error: %s\n", SOIL_last_result());

		Invalidate();
		return;
	}

	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &Width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &Height);
	TextureId = (ImTextureID)(intptr_t)tex;

	LeakCount++;
}

RenderImage::RenderImage(RenderImage&& other) 
{
	Width = other.Width;
	Height = other.Height;
	TextureId = other.TextureId;

	other.Invalidate();
}

RenderImage::~RenderImage()
{
	if (IsValid())
	{
		GLuint id = (GLuint)TextureId;
		glDeleteTextures(1, &id);
		--LeakCount;
	}

	Invalidate();
}

void RenderImage::DebugAssertLeaks()
{
	ImageCache::Clear();
	if (LeakCount)
		throw std::runtime_error("Leaking images !");
}

using CacheEntry = std::pair<std::string, ImageRef>;
std::vector<CacheEntry> ImagePool;

static auto HasString(const std::string& str)
{
	auto res = std::find_if(ImagePool.begin(), ImagePool.end(), 
		[&str](const CacheEntry& pair) { return pair.first == str; });
	return res;
}

static void PopFirst()
{
	LOGf("Pool full, popping %s\n", ImagePool[0].first.c_str());
	ImageCache::FreeImage(ImagePool[0].first);
}

static void AddValue(const std::string& str, ImageRef img)
{
	ImagePool.emplace_back(str, img);
	LOGf("Pushing %s size %lu\n", str.c_str(), ImagePool.size());

	const u32 MaxCachedImages = UseLowMemory ? 3 : 8;
	if (ImagePool.size() > MaxCachedImages)
		PopFirst();
}

void ImageCache::Clear()
{
	ImagePool.clear();
}

void ImageCache::FreeImage(const std::string &img)
{
	auto res = HasString(img);
	if (res == ImagePool.end()) return;
	ImagePool.erase(res);
}

ImageRef ImageCache::LoadDDS(const std::vector<u8> &data, const std::string &name)
{	
	auto res = HasString(name);
	if (res != ImagePool.end())
		return res->second;

	auto image = std::make_shared<RenderImage>(data);

	if (image->IsValid())
		AddValue(name, image);
	
	return image;
}

IPage::~IPage(){}
IUIControlObj::~IUIControlObj(){}
