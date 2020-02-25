#include <cmath>

//Just some basic operations on quaternions

struct Quaternion {
public:
	double w, i, j, k;

	Quaternion(double w, double i, double j, double k){
		this->w = w;
		this->i = i;
		this->j = j;
		this->k = k;
	}

	// Could be operator, but
	// 1. I wanted to avoid confusion with normal numbers
	// 2. I just copied it from my previous code in Java, where there are no operators :)
	Quaternion add(const Quaternion &a){
    	return Quaternion(w + a.w, i + a.i, j + a.j, k + a.j);
	}

	Quaternion multiply(const Quaternion &b){
		return Quaternion(w * b.w - i * b.i - j * b.j - k * b.k,  // 1
			w * b.i + i * b.w + j * b.k - k * b.j,  // i
			w * b.j - i * b.k + j * b.w + k * b.i,  // j
			w * b.k + i * b.j - j * b.i + k * b.w);   // k
	}

	Quaternion exp(){
		double mag = std::sqrt(i * i + j * j + k * k);
		if(mag == 0) return Quaternion(std::exp(w), 0, 0, 0);
		double sinmag = sin(mag) / mag;
		double ew = std::exp(w);
		return Quaternion(ew * std::cos(mag), ew * i * sinmag, ew * j * sinmag, ew * k * sinmag);
	}

	Quaternion cos(){
		double mag = std::sqrt(i * i + j * j + k * k);
		if (mag == 0) return Quaternion(std::cos(w), 0, 0, 0);

		double t = -std::sin(w) * std::sinh(mag) / mag;
		return Quaternion(std::cos(w) * std::cosh(mag), t * i, t * j, t * k);
	}
  
	double magnitude(){
		return std::sqrt(w * w + i * i + j * j + k * k); 
	}
};