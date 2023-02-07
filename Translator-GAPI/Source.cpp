#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include "nlohmann/json.hpp"

#define CURL_STATICLIB

#include "curl/curl.h"

std::size_t callback(const char* in, std::size_t size, std::size_t num, std::string* out)
{
	const std::size_t totalBytes(size * num);
	out->append(in, totalBytes);
	return totalBytes;
}

std::string Encode(const std::string& in)
{
	std::ostringstream out;
	out.fill('0');
	out << std::hex;
	for (const auto& c : in)
	{
		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
			out << c;
			continue;
		}

		out << '%' << std::uppercase << std::setw(2) << int((unsigned char)c) << std::nouppercase;
	}
	return out.str();
}

std::string FormatLanguageDetectionQuery(const std::string& phrase)
{
	std::ostringstream out;
	out << "q=" << Encode(phrase);
	return out.str();
}

std::string FormatTranslationQuery(const std::string& phrase, const std::string& sourceLanguage, const std::string& targetLanguage = "en")
{
	std::ostringstream out;
	out << "q=" << Encode(phrase) << "&target=" << targetLanguage << "&source=" << sourceLanguage;
	return out.str();
}

int main()
{
	std::cout << "Enter what you want to translate to english (auto language detection): ";
	std::string in;
	std::getline(std::cin, in);

	std::unique_ptr<std::string> pTranslatedJSONString = std::make_unique<std::string>();
	std::unique_ptr<std::string> pDetectedLanguageJSONString = std::make_unique<std::string>();

	CURL* hnd = curl_easy_init();

	curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");

	struct curl_slist* headers = NULL;
	headers = curl_slist_append(headers, "content-type: application/x-www-form-urlencoded");
	headers = curl_slist_append(headers, "Accept-Encoding: application/gzip");
	headers = curl_slist_append(headers, "X-RapidAPI-Key: ---API KEY HERE---");
	headers = curl_slist_append(headers, "X-RapidAPI-Host: google-translate1.p.rapidapi.com");
	curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);

	//DETECTING LANGUAGE
	curl_easy_setopt(hnd, CURLOPT_COPYPOSTFIELDS, FormatLanguageDetectionQuery(in).c_str());
	curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, callback);
	curl_easy_setopt(hnd, CURLOPT_WRITEDATA, pDetectedLanguageJSONString.get());
	curl_easy_setopt(hnd, CURLOPT_URL, "https://google-translate1.p.rapidapi.com/language/translate/v2/detect");
	[[maybe_unused]] CURLcode languageDetectionCode = curl_easy_perform(hnd);
	//Parsing JSON
	nlohmann::json detectedLanguageJSON = nlohmann::json::parse(*pDetectedLanguageJSONString);
	std::string detectedLanguage = detectedLanguageJSON["data"]["detections"][0][0]["language"].get<std::string>();

	//Translating
	curl_easy_setopt(hnd, CURLOPT_COPYPOSTFIELDS, FormatTranslationQuery(in, detectedLanguage).c_str());
	curl_easy_setopt(hnd, CURLOPT_WRITEDATA, pTranslatedJSONString.get());
	curl_easy_setopt(hnd, CURLOPT_URL, "https://google-translate1.p.rapidapi.com/language/translate/v2");
	[[maybe_unused]] CURLcode translationCode = curl_easy_perform(hnd);

	nlohmann::json translatedJSON = nlohmann::json::parse(*pTranslatedJSONString);
	std::cout << "The translated sentence from \"" << detectedLanguage << "\" (detected automatically) is: " << translatedJSON["data"]["translations"][0]["translatedText"].get<std::string>() << std::endl;
	return 0;
}