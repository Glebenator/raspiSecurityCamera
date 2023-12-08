#include <iostream>
#include <string>
#include <curl/curl.h>

int main() {
    CURL *curl;
    CURLcode res = CURLE_OK;

    curl = curl_easy_init();
    if(curl) {
        const std::string username = "lilweeb420x69@gmail.com";
        const std::string password = "qTkgfJyC4tt8ft";
        const std::string from = "<lilweeb420x69@gmail.com>";
        const std::string to = "<ggladyshevsky@gmail.com>";
        const std::string url = "smtp.gmail.com:25"; // Use the correct SMTP server and port

        // Set necessary options
        curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from.c_str());

        struct curl_slist *recipients = nullptr;
        recipients = curl_slist_append(recipients, to.c_str());
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        const std::string emailData =
            "To: " + to + "\r\n"
            "From: " + from + "\r\n"
            "Subject: Simple test\r\n"
            "\r\n" // empty line to divide headers from body
            "This is a simple test email sent from a C++ program.\r\n";

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, emailData.c_str());

        // Send the email
        res = curl_easy_perform(curl);

        // Check for error
        if(res != CURLE_OK){
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        std::cout<< "email sent? " << std::endl;
        // Free the list of recipients
        curl_slist_free_all(recipients);

        // Always cleanup
        curl_easy_cleanup(curl);
    }

    return 0;
}