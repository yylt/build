#!/bin/bash
set -x
dir=$(dirname "$(readlink -f $0)")
fname=test-images
fpath=$dir/$fname


docker login

function exist()
{
    repo=$1
    docker pull $1
    if [ $? -eq 0 ];then
        echo 0
        return
    else
        echo 1
        return
    fi
}


while IFS= read -r line; do
    registry=$(echo -n  $line |cut -d / -f 1)
    name=${line##*/}
    newrepo="docker.io/yylt/arm64-$name"

    if [ $registry = 'docker.io' ];then
       continue
    else
        if [ $(exist $newrepo) -eq 0 ];then
            continue
        fi    
        docker pull --platform linux/arm64 $line
        if [ $? -eq 0 ] ;then
            docker tag "$line" $newrepo
            docker push $newrepo
            if [ $? -ne 0 ] ;then
                exit -1
            fi
        fi
    fi   
done < $fpath

while IFS= read -r line; do
    registry=$(echo -n  $line |cut -d / -f 1)
    name=${line##*/}
    newrepo="docker.io/yylt/$name"

    if [ $registry = 'docker.io' ];then
       continue
    else
        if [ $(exist $newrepo) -eq 0 ];then
            continue
        fi    
        docker pull --platform linux/amd64 $line
        if [ $? -eq 0 ] ;then
            docker tag "$line" $newrepo
            docker push $newrepo
            if [ $? -ne 0 ] ;then
                exit -1
            fi
        fi
    fi   
done < $fpath