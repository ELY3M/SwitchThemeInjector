#pragma once
#include "../UI/UI.hpp"

class ImagePreview : public IUIControlObj
{
	public:
		ImagePreview(ImageRef img, std::string label);

		void Render(int X, int Y) override;		
		void Update() override {};

	private:
		ImageRef img;
		std::string label;
};
