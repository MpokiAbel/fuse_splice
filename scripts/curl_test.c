#include <stdio.h>
#include <curl/curl.h>

/* This is a simple example showing how to list the folders within an IMAP
 * mailbox.
 *
 * Note that this example requires libcurl 7.30.0 or above.
 */

int main(void)
{
    CURL *curl;
    CURLcode res = CURLE_OK;

    curl = curl_easy_init();
    if (curl)
    {
        /* Set username and password */
        curl_easy_setopt(curl, CURLOPT_USERNAME, "mpoki.mwaisela@grenoble-inp.org");
        curl_easy_setopt(curl, CURLOPT_PASSWORD, "Cl@ssof21_Udsm");

        /* This will list the folders within the user's mailbox. If you want to
         * list the folders within a specific folder, for example the inbox, then
         * specify the folder as a path in the URL such as /INBOX */
        curl_easy_setopt(curl, CURLOPT_URL, "imaps://webmail.grenoble-inp.org/");

        /* Perform the list */
        printf("Before Curls\n");
        res = curl_easy_perform(curl);

        /* Check for errors */
        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

        /* Always cleanup */
        curl_easy_cleanup(curl);
    }

    return (int)res;
}