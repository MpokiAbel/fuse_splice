#include <stdio.h>
#include <curl/curl.h>

int main(int argc, char const *argv[])
{
    CURL *curl = curl_easy_init();
    CURLcode res = CURLE_OK;
    if (curl)
    {
        curl_socket_t sockfd;
        curl_easy_setopt(curl, CURLOPT_URL, "imaps://webmail.grenoble-inp.org/");
        curl_easy_setopt(curl, CURLOPT_USERNAME, "mpoki.mwaisela@grenoble-inp.org");
        curl_easy_setopt(curl, CURLOPT_PASSWORD, "Cl@ssof21_Udsm");
        /* Do not do the transfer - only connect to host */
        curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);
        res = curl_easy_perform(curl);

        /* Extract the socket from the curl handle */
        res = curl_easy_getinfo(curl, CURLINFO_ACTIVESOCKET, &sockfd);

        if (res != CURLE_OK)
        {
            printf("Error: %s\n", curl_easy_strerror(res));
            return 1;
        }
    }
    return 0;
}
