/*
 * 从www.mt-bbs.com中的特定html页面解析原图片链接并自动下载到本地
 * 作者：huolinliang@gmail.com
 * 时间：2014-10-09
 * 部分代码搬自网络,仅用于个人试验
 */

#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <time.h>

//TODO:Auto login, auto search html file in web page and download images

int DownloadImg();
char *GetMidStr(const char*string,const char *left,const char *right,const char *div);
size_t WriteData(void *ptr, size_t size, size_t nmemb, FILE *stream);

int main(int argc, char *argv[])
{
    time_t start, finish;
    DIR *topdir, *subdir;
    struct dirent *ptr_topdir, *ptr_subdir;
    int fd_htmlfile, fd_tmpfile, size_htmlfile, tmp_chdir;
    struct stat buf;
    char *buffer, *url_result;
    char *left = "zoomfile=\"";
    char *right = "\" ";
    
    start = time(NULL);
    topdir = opendir("./");
    while ((ptr_topdir = readdir(topdir)) != NULL) {
        const char *subdir_name = ptr_topdir->d_name;
        if (subdir_name[0] == '.' && (subdir_name[1] == 0 || (subdir_name[1] == '.' && subdir_name[2] == 0))) 
            continue;
        tmp_chdir = chdir(subdir_name);
        if (tmp_chdir == 0) {
            subdir = opendir("./");
            while ((ptr_subdir = readdir(subdir)) != NULL) {
                const char *html_file = ptr_subdir->d_name;
                //Find html file in each subdir.
                if (!(strstr(html_file, ".htm") || strstr(html_file, ".html")))
                    continue;
                printf("\tWorking on %s/%s\n", subdir_name, html_file);
                fd_htmlfile = open(html_file, O_RDONLY);
                if (fd_htmlfile < 0) {
                    printf("open %s failed\n", html_file);
                    continue;
                }
                else {
                    fstat(fd_htmlfile, &buf);
                    size_htmlfile = buf.st_size;
                    buffer = malloc(size_htmlfile);
                    if (!buffer) {
                        printf("Failed to kmalloc buffer.\n");
                        continue;
                    }
                    read(fd_htmlfile, buffer, size_htmlfile); 
                    url_result = GetMidStr(buffer, left, right, "\n");
                    fd_tmpfile = open("./tmpfile", O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
                    write(fd_tmpfile, url_result, strlen(url_result));
                    close(fd_tmpfile);
                    DownloadImg();
                }
            }
            //Delete tmpfile before go back to upper dir.
            remove("./tmpfile");
            chdir("../");
        }
    }
    closedir(topdir);
    finish = time(NULL);
    printf("\tTotally takes %ld seconds\n", finish - start);
    return 0;
}

char *GetMidStr(const char *string, const char *left, const char *right, const char *div)
{
    int string_len, left_len, right_len, div_len, i, flag, start, end, result_len, m_flag;
    char *result,*temp;
    flag = 0;
    m_flag = 0;
    string_len = strlen(string);
    left_len = strlen(left);
    right_len = strlen(right);
    div_len = strlen(div);
    for(i = 0; i < string_len; i++) {
        //Find the left part.
        if (*(string+i) == *left && flag == 0) {
            if (strncmp(string+i, left, left_len) == 0) {
                flag = 1;
                start = i;
            }
        }
        //Find the right part.
        if (*(string+i) == *right && flag == 1) {
            if (strncmp(string+i, right, right_len) == 0) {
                flag = 2;
                end = i;
            }
        }
        //Get the resutl and set flag to 0.
        if (flag == 2) {
            result_len = end - start - left_len;
            flag = 0; //For complex match
            if (m_flag == 0) {
                result = (char*)malloc(result_len + 1);
                *(result+result_len) = 0;
                strncpy(result, string + start + left_len, result_len);
                m_flag = 1;
            }
            else {
                temp = (char*)malloc(strlen(result) + result_len + div_len + 1);
                *(temp+strlen(result) + result_len + div_len) = 0;
                strncpy(temp, result, strlen(result));
                strncpy(temp + strlen(result), div, div_len);
                strncpy(temp + strlen(result) + div_len, string+start + left_len, result_len);
                result = temp;
            }
        }
    }
    return result;
}

size_t WriteData(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written;
    written = fwrite(ptr, size, nmemb, stream);
    return written;
}

int DownloadImg()
{
    FILE *fp_tmpfile, *fp_output;
    char img_address[128], output_filename[16]="";
    int img_address_len;
    CURL *curl;
    CURLcode res;
    if ((fp_tmpfile = fopen("./tmpfile", "r")) == NULL) {
        printf("open tmpfile error\n");
        return -1;
    }
    while (!feof(fp_tmpfile)) {
        fgets(img_address, 128, fp_tmpfile);
        img_address_len = strlen(img_address);
        strncpy(output_filename, img_address + 60, 8);
        strcat(output_filename, ".jpg");
        printf("%s\n", output_filename);
        curl = curl_easy_init();
        if (curl) {
            fp_output = fopen(output_filename, "wb");
            curl_easy_setopt(curl, CURLOPT_URL, img_address);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteData);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp_output);
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            fclose(fp_output);
        }
        strncpy(output_filename, "", 16);
    }
    fclose(fp_tmpfile);
}
