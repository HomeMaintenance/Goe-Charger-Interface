#include <iostream>
#include <curl/curl.h>
#include <json/json.h>
#include <GoeCharger.h>


std::size_t callback(
        const char* in,
        std::size_t size,
        std::size_t num,
        std::string* out)
{
    const std::size_t totalBytes(size * num);
    out->append(in, totalBytes);
    return totalBytes;
}

int test_curl(){
    const std::string url("http://date.jsontest.com/");

    CURL* curl = curl_easy_init();

    // Set remote URL.
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    // Don't bother trying IPv6, which would increase DNS resolution time.
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

    // Don't wait forever, time out after 10 seconds.
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);

    // Follow HTTP redirects if necessary.
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // Response information.
    long httpCode(0);
    std::unique_ptr<std::string> httpData(new std::string());

    // Hook up data handling function.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);

    // Hook up data container (will be passed as the last parameter to the
    // callback handling function).  Can be any pointer type, since it will
    // internally be passed as a void pointer.
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, httpData.get());

    // Run our HTTP GET command, capture the HTTP response code, and clean up.
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);
    if (httpCode == 200)
    {
        std::cout << "\nGot successful response from " << url << std::endl;

        // Response looks good - done using Curl now.  Try to parse the results
        // and print them out.
        Json::Value jsonData;
        Json::Reader jsonReader;

        if (jsonReader.parse(*httpData.get(), jsonData))
        {
            std::cout << "Successfully parsed JSON data" << std::endl;
            std::cout << "\nJSON data received:" << std::endl;
            std::cout << jsonData.toStyledString() << std::endl;

            const std::string dateString(jsonData["date"].asString());
            const std::size_t unixTimeMs(
                    jsonData["milliseconds_since_epoch"].asUInt64());
            const std::string timeString(jsonData["time"].asString());

            std::cout << "Natively parsed:" << std::endl;
            std::cout << "\tDate string: " << dateString << std::endl;
            std::cout << "\tUnix timeMs: " << unixTimeMs << std::endl;
            std::cout << "\tTime string: " << timeString << std::endl;
            std::cout << std::endl;
        }
        else
        {
            std::cout << "Could not parse HTTP data as JSON" << std::endl;
            std::cout << "HTTP data was:\n" << *httpData.get() << std::endl;
            return 1;
        }
    }
    else
    {
        std::cout << "Couldn't GET from " << url << " - exiting" << std::endl;
        return 1;
    }

    return 0;
}


void set_alw(goe::Charger& charger){
    auto alw1 = charger.get_alw();
    charger.set_alw(!alw1);
    auto alw2 = charger.get_alw();
    charger.set_alw(!alw2);
    std::this_thread::sleep_for(std::chrono::milliseconds(2500));
    charger.set_alw(false);
}

void set_amp(goe::Charger& charger){
    auto start = charger.get_amp();
    for(int i = 6; i <= 20; ++i){
        charger.set_amp(i);
        std::cout << charger.get_amp() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
    }
    charger.set_amp(6);
    std::cout << charger.get_amp() << std::endl;
}

void test_goe(){
    goe::Charger charger("charger","192.168.178.106");
    set_amp(charger);
    set_alw(charger);

    // auto amp = charger.get_amp();
}

int main(int argc, char * argv[]){
    #ifdef GOE_DEBUG
    assert(false);
    #endif
    test_goe();
    return 0;
}
