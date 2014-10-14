#!/bin/bash
####################################################
#    Info: download images from www.mt-bbs.com     #
#    Author: Leung                                 #
#    Date: 20141014                                #
####################################################

time_all_start=$(date +%s)

# go into each directory and do the next
for j in $(ls -d */);do
    cd $j
    echo -e "\n\tMission $j start."

    if [ ! -d image_save ]; then
        mkdir image_save
    fi

    # check if any htm* file exist in each directory
    ls ./*.htm* 1>/dev/null 2>&1
    if [ $? -eq 0 ]; then
        for i in $(ls ./*.htm* | awk '{print $1;}');do
            cat $i | grep zoomfile | awk -F '="' '{print $7}' | sed "s/\" file//g" >> zoomfile_line
        done

        # batch download with wget
        cat zoomfile_line |
        while read row; do
            sleep 0.2
            wget -b -N -c -P image_save $row -a ./wget.log 
        done

        # check if download finished, and show images info
        while :
        do
            line_sum=$(cat wget.log | wc -l)
            sleep 5 
            line_sum2=$(cat wget.log | wc -l)
            if [ $line_sum -eq $line_sum2 ]; then
                echo -e "\n\tMission $j completed."
                img_num=$(ls image_save/ | wc -l)
                img_size=$(du -sh image_save/ | awk '{print $1}')
                echo -e "\tThere are $img_num pictures in $j."
                echo -e  "\tDirectory size : $img_size"
                echo ""
                break
            else
                echo -e "\tStill downloading...keep waiting..."
            fi
        done

        # remove all temporary files we created
        rm zoomfile_line
        rm wget.log
    else
        echo -e "\tFailed: There is no html file in $j\n"
    fi
    cd - 1>/dev/null
done

time_all_end=$(date +%s)
echo -e "\tTotally takes $(( $time_all_end - $time_all_start )) seconds.\n"


