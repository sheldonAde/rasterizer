// ÎÆÀí¼ÓÔØ£¨stb_image£©
#include "renderer/texture.hpp"
#include <cstdio>


bool Texture2D::load(const char* path) {
	int comp = 0; stbi_uc* img = stbi_load(path, &w, &h, &comp, 4);
	if (!img) return false;
	data.assign(img, img + (size_t)w * h * 4);
	stbi_image_free(img); c = 4; return true;
}