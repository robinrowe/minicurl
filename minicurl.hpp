//sudo apt-get install -y curl libcurl4-openssl-dev

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Minicurl
// Copyright 2019 Jean Diogo (aka Jango) <jeandiogo@gmail.com>
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// minicurl.hpp
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef MINICURL_HPP
#define MINICURL_HPP

#include <cassert>
#include <cstring>
#include <vector>
#include <string>
#include <curl/curl.h>
	
class curl
{
	class chunk
	{
		public:
		
		std::size_t size;
		char * data;
		
		friend void swap(chunk & x, chunk & y)
		{
			using std::swap;
			swap(x.size, y.size);
			swap(x.data, y.data);
		}
		
		chunk(std::size_t s = 0) : size(s)
		{
			data = (char *) malloc(sizeof(char) * (size + 1));
			if(data) data[size] = '\0';
			else size = 0;
		}
		
		chunk(chunk const & c) : size(c.size)
		{
			data = (char *) malloc(sizeof(char) * (size + 1));
			if(data)
			{
				memcpy(data, c.data, size);
				data[size] = '\0';
			}
			else size = 0;
		}
		
		chunk(chunk && c) : chunk() {swap(*this, c);}
		chunk & operator=(chunk c) {swap(*this, c); return *this;}
		~chunk() {if(data) free(data);}
	};

	static std::size_t curl_write_function(void * contents, std::size_t size, std::size_t memory, void * pointer)
	{
		std::size_t realsize = size * memory;
		chunk * mem = (chunk *) pointer;
		char * ptr = (char *) realloc(mem->data, mem->size + realsize + 1);
		if(!ptr) return 0;
		mem->data = ptr;
		memcpy(&(mem->data[mem->size]), contents, realsize);
		mem->size += realsize;
		mem->data[mem->size] = 0;
		return realsize;
	}
	
	CURL * inner_curl;
	
	curl() : inner_curl(nullptr)
	{
		curl_global_init(CURL_GLOBAL_ALL);
		inner_curl = curl_easy_init();
		assert(inner_curl);
		curl_easy_setopt(inner_curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(inner_curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
		curl_easy_setopt(inner_curl, CURLOPT_WRITEFUNCTION, curl_write_function);
	}
		
	static auto & instantiate()
	{
		static curl singleton;
		return singleton;
	}
	
	auto fetch(std::string const & url, std::string const & payload = "", std::vector<std::string> const & headers = {})
	{
		auto response = std::string("");
		struct curl_slist * header = nullptr;
		for(auto const & i : headers) header = curl_slist_append(header, i.c_str());
		curl_easy_setopt(inner_curl, CURLOPT_HTTPHEADER, header);
		curl_easy_setopt(inner_curl, CURLOPT_URL, url.c_str());
		if(payload.size()) curl_easy_setopt(inner_curl, CURLOPT_POSTFIELDS, payload.c_str());
		chunk raw_response;
		curl_easy_setopt(inner_curl, CURLOPT_WRITEDATA, (void *) &raw_response);
		if(curl_easy_perform(inner_curl) == CURLE_OK && raw_response.size) response = std::string(raw_response.data, raw_response.size);
		curl_slist_free_all(header);
		return response;
	}
	
	public:
	
	curl(curl const &) = delete;
	curl(curl &&) = delete;
	curl & operator=(curl) = delete;
	
	~curl()
	{
		if(inner_curl) curl_easy_cleanup(inner_curl);
		curl_global_cleanup();
	}
	
	static auto get(std::string const & url, std::string const & header = "")          {return instantiate().fetch(url, "", {header});}
	static auto get(std::string const & url, std::vector<std::string> const & headers) {return instantiate().fetch(url, "", headers);}
	
	static auto post(std::string const & url, std::string const & payload = "", std::string const & header = "")     {return instantiate().fetch(url, payload, {header});}
	static auto post(std::string const & url, std::string const & payload, std::vector<std::string> const & headers) {return instantiate().fetch(url, payload, headers);}
};

#endif