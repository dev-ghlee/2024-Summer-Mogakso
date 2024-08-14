// BVH_Loader.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define JGL2_IMPLEMENTATION
#include <iostream>
#include <cmath>
#include <Eigen/Core>
#include <JGL2/JGL.hpp>
#include <JGL2/Anim3DView.hpp>

using namespace JGL2;
using namespace jm;

Anim3DView<JR::PBRRenderer>* view = nullptr;

enum CHANNEL
{
	XPOS,
	YPOS, 
	ZPOS,
	XROT,
	YROT,
	ZROT,
};


struct link {
	std::vector<CHANNEL> channels;
	mat4 trans = mat4(1);
	mat4 globalTransform = mat4(1);
	vec3 l;
	std::string name;
	int parent = -1;
	link(std::string n, std::vector <CHANNEL> c, vec3 off, int p)
		: name(n), channels(c), l(off), parent(p) {}
	
	void update(const std::vector<float>&data, int off, mat4 parentTransform)
	{
		trans = mat4(1);
		for (auto i = 0; i < channels.size(); i++)
		{
			mat4 tr = mat4(1);
			switch (channels[i])
			{
			case XPOS:
				tr = translate(vec3(data[off+i]*5,0,0));
				break;
			case YPOS:
				tr = translate(vec3(0,data[off + i]*5, 0));
				break;
			case ZPOS:
				tr = translate(vec3(0,0,data[off + i]*5));
				break;
			case XROT:
				tr = rotate(data[off+i]*PI/180,vec3(1,0,0));
				break;
			case YROT:
				tr = rotate(data[off + i] * PI / 180, vec3(0, 1, 0));
				break;
			case ZROT:
				tr = rotate(data[off + i] * PI / 180, vec3(0, 1, 0));
				break;
			}
			trans = trans * tr;
			
		}
		trans = translate(l) * trans;
		globalTransform = parentTransform * trans;
	}
	mat4 getGlobalTransform() const { return globalTransform; }

	void render() const {
		
		vec3 pt = globalTransform * vec4(0, 0, 0, 1); // ÀÚ±â point
		if (parent >= 0)
		{
			vec3 pr = (globalTransform*inverse(trans)) * vec4(0, 0, 0, 1);
			JR::drawCylinder(pr, pt, 0.7, vec4(1, 0, 0, 1));
		}
		JR::drawSphere(pt, 1, vec4(.1, .1, .1, 1));
	}
};

struct Body {
	std::vector <link> links;
	int frames = 0;
	float frameRate = 0;
	int nChannels = 0;
	std::vector<std::vector<float>> data;

	void loadEndSite(std::vector<link>& links, std::istream& is, int p)
	{
		std::string tmp, name;
		vec3 offset;
		is >> tmp; //Site
		is >> name;
		is >> tmp; // {
		is >> tmp; // OFFSET
		is >> offset.x >> offset.y >> offset.z;
		links.emplace_back(name, std::vector<CHANNEL>(), offset, p);
		is >> tmp; // }
	}

	void loadJoint(std::vector<link>& links, std::istream& is, int p)
	{
		std::string tmp, name, chname;
		std::vector<CHANNEL> chs;
		vec3 offset;
		int nch;
		is >> name;
		is >> tmp; // {
		is >> tmp; // OFFSET
		is >> offset.x >> offset.y >> offset.z;
		offset *= 5;
		is >> tmp; //CHANNELS
		is >> nch;
		for (int i = 0; i < nch; i++)
		{
			is >> chname;
			if (chname == "Xposition") chs.push_back(XPOS);
			else if (chname == "Yposition") chs.push_back(YPOS);
			else if (chname == "Zposition") chs.push_back(ZPOS);
			else if (chname == "Xrotation") chs.push_back(XROT);
			else if (chname == "Yrotation") chs.push_back(YROT);
			else if (chname == "Zrotation") chs.push_back(ZROT);
		}
		links.emplace_back(name, chs, offset, p);
		int me = links.size() - 1;
		while (1) {
			is >> tmp; // JOINT or END
			if (tmp == "JOINT") loadJoint(links, is, me);
			else if (tmp == "END") loadEndSite(links, is, me);
			else if (tmp == "}") break;
		}
	}

	void loadBVH(std::istream& is)
	{
		clear();
		std::string tmp;
		is >> tmp; // HIERARCHY
		is >> tmp; // ROOT
		loadJoint(links, is, -1);

		is >> tmp; //MOTION
		is >> tmp; //Frames:
		is >> frames;
		is >> tmp; //Frame
		is >> tmp; //Time:
		is >> frameRate;

		data.resize(frames);
		for (int i = 0; i < links.size(); i++)
		{
			nChannels += links[i].channels.size();
		}
		for (int f = 0; f < frames; f++)
		{
			data[f].resize(nChannels);
			for (int i = 0; i < nChannels; i++)
			{
				is >> data[f][i];
			}
		}

	}

	void clear()
	{
		links.clear();
	}
	void render()
	{

		for (int i = 0; i < links.size(); i++)
		{
			links[i].render();
		}
	}

	void update(int fr)
	{
		int off = 0;
		for (int i = 0; i < links.size(); i++)
		{
			links[i].update(data[fr],off, links[i].parent>=0?links[links[i].parent].getGlobalTransform():mat4(1));
			off += links[i].channels.size();
		}
	}
};

Body body;

void load(Widget* ,void*,const std::vector<std::string>& files)
{
	std::ifstream ifs(files[0]);
	body.loadBVH(ifs);
	ifs.clear();
	view->range(body.frames);
	view->fps(1.f / body.frameRate);
}

void init()
{
	body.clear();
}

void frame(long fr)
{
	body.update(fr);
}

//std::vector<link> links;
//int picked = -1;




void render() {
	body.render();
	
	JR::drawQuad(jm::vec3(0), jm::vec3(0, 1, 0), jm::vec2(1000), jm::vec4(0, 0, .4, 1));
}

bool move3D(const vec3& p) {
	return false;
}

bool drag3D(const vec3& delta) {
	return false;
}

int main() {

	Window* win = new Window(800, 600, "IK");
	win->alignment(align_t::ALL);
	view = new Anim3DView<JR::PBRRenderer>(0, 0, 800, 600, "View");
	view->move3DCB(move3D);
	view->drag3DCB(drag3D);
	view->frameCB(frame);
	view->renderFunc(render);
	view->dndCallback(load);
	win->show();
	_JGL::run();
	return 0;
}

