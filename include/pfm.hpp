//
//  pfm.hpp
//  JGLTest
//
//  Created by Hyun Joon Shin on 2022/05/10.
//

#ifndef pfm_h
#define pfm_h

#include <fstream>
#include <glm/glm.hpp>

namespace pf {
	inline std::tuple<int,int,int,float*> loadPFM( const std::string& fn ) {
		char buf[1024];
		int w=0, h=0, c=0;
		float endianness;

		std::ifstream file(fn, std::ios::binary);
		if(!file.is_open() ) throw "File not found";
		file.getline(buf, 1024);
		if( buf[0] != 'P' ) throw "Not supported file";
		if( buf[1] == 'F' ) c = 3;
		else c = 1;
		file >> w >> h >> endianness;
		printf("Image: %d %d %f <%d>\n", w, h, endianness, c);
		file.getline(buf, 1024);
	
		float* data = new float[w*h*c];
		if( endianness<0 ) /*little endian <native> */ {
			for( int y=0; y<h; y++ ) {
				file.read((char*)(data+(h-1-y)*w*c), w*c*sizeof(float));
			}
		}
		else	/* Big endian <need conversion> */ {
			char bigBuf[4];
			char littleBuf[4];
			for( int y=0; y<h; y++ ) {
				for( int i=0; i<w*c; i++) {
					file.read(bigBuf,4);
					littleBuf[0] = bigBuf[3];
					littleBuf[1] = bigBuf[2];
					littleBuf[2] = bigBuf[1];
					littleBuf[3] = bigBuf[0];
					data[i+(h-1-y)*w*c] = *((float*)littleBuf);
				}
			}
		}
	
		file.close();
		return std::make_tuple(w,h,c,data);
	}

	inline void savePFM(const std::string& fn, size_t w, size_t h, float* data, bool flip = true ) {
		std::ofstream ofs( fn, std::ios::binary );
		ofs<<"PF\n"<<w<<" "<<h<<"\n-1.0\n";
		if( flip )
			for( int y=0; y<h; y++)
				ofs.write((char*)(data+w*3*(h-1-y)),w*3*sizeof(float));
		else
			for( int y=0; y<h; y++)
				ofs.write((char*)(data+w*3*(y)),w*3*sizeof(float));

		ofs.close();
	}


	inline unsigned char* convertHDR2sRGB( int n, float* data, float exp=1 ) {
		unsigned char* ldr = new unsigned char[n];
		for( int i=0; i<n; i++ ) {
			float d = glm::clamp( data[i] * exp, 0.f, 1.f );
			d = d>0.0031308f?1.055f*powf(d,1/2.4f):12.92f*d;
			ldr[i] = glm::clamp(int(d*255),0,255);
		}
		return ldr;
	}

	inline void convertHDR2LDR( int n, float* data, unsigned char* ldr, float exp=1 ) {
		for( int i=0; i<n; i++ ) {
			float d = glm::clamp( data[i] * exp, 0.f, 1.f );
			ldr[i] = glm::clamp(int(d*255),0,255);
		}
	}

	inline void convertHDR2sRGB( int n, float* data, unsigned char* ldr, float exp=1 ) {
		for( int i=0; i<n; i++ ) {
			float d = glm::clamp( data[i] * exp, 0.f, 1.f );
			d = d>0.0031308f?1.055f*powf(d,1/2.4f):12.92f*d;
			ldr[i] = glm::clamp(int(d*255),0,255);
		}
	}

	inline std::tuple<int,int,float*> cropCenter( int w, int h, int c, const float* dt, int desiredW, int desiredH ) {
		int roiw = std::min(desiredW,w);
		int roih = std::min(desiredH,h);
		int roix = std::max(0,w/2-roiw/2);
		int roiy = std::max(0,h/2-roih/2);
		float* buf = new float[roiw*roih*c];
		for( int y=0; y<roih; y++ )
			memcpy( (buf+y*roiw*c), (dt+((y+roiy)*w+roix)*c), roiw*c*sizeof(float));
		return std::make_tuple( roiw, roih, buf );
	}
}



#endif /* pfm_h */
