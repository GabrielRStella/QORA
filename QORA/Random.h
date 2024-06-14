#pragma once

class Random
{
	std::default_random_engine eng;
	std::default_random_engine::result_type current_seed;
public:
	Random();
	//
	void seed(std::default_random_engine::result_type seed);
	void seed_time(std::default_random_engine::result_type offset = 0); //set seed using current time
	std::default_random_engine::result_type get_seed() const;
	//
	int random_int(int end); //random int in [0, end)
	int random_int(int a, int b); //random int in [a, b] inclusive
	double random_uniform(); //uniform random in [0, 1)
	//
	template<typename T>
	const T& sample(const std::vector<T>& vec);
	template<typename T>
	void shuffle(std::vector<T>& vec);
};

template<typename T>
inline const T& Random::sample(const std::vector<T>& vec)
{
	int n = vec.size();
	return vec.at(random_int(0, n - 1));
}

template<typename T>
inline void Random::shuffle(std::vector<T>& vec)
{
	int n = vec.size();
	for (int i = 1; i < n; i++) {
		int j = random_int(i);
		std::swap(vec[i], vec[j]);
	}
}
