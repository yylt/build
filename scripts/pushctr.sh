#!/bin/bash

ERRUSE=-1
Addr="/run/containerd/containerd.sock"
Image=""
Ouputs=""
Archs=""
UserPw=""
function usage()
{
echo -e "
$PWD/$(basename $0) usage:
\t -h: print help
\t -i: pull image, should be valid image.
\t -u: user and password, example user:passwd.
\t -a: containerd sock address.
\t -o: Comma-segmentation, push images, example image1,image2.
\t -r: Comma segmentation, images arch, should onebyone with images, example linux/amd64,linux/arm64.
"
exit $ERRUSE
}


while getopts ":i:u:a:o:r:h" Opt; do
    case $Opt in
    i ) Image="${OPTARG}" ;;
    u ) User="${OPTARG}" ;;
    a ) Addr="${OPTARG}" ;;
    o ) Outputs="${OPTARG}" ;;
    r ) Archs="${OPTARG}" ;;
    h ) usage ;;
    * ) usage ;;
    esac
done
if [ -z "${Image}"  ]; then
   echo "Delete image must set"
   usage && exit $ERRUSE
fi
if [ -z "${Outputs}"  ]; then
   echo "output image must set"
   usage && exit $ERRUSE
fi
if [ -z "${Archs}"  ]; then
   echo "output image arch must set"
   usage && exit $ERRUSE
fi
if [ ! -z "${User}" ]; then
   echo "user password set $User"
   UserPw="-u $User"
fi

if [ -z "${Addr}" ]; then
   Addr="/run/containerd.sock"
fi

CTR="ctr -a $Addr"
Images=$(echo $Outputs | tr "," "\n")
Images=($Images)
Archs=$(echo $Archs | tr "," "\n")
Archs=($Archs)

# if outputs length 1, and arch is 0 
# set default arch linux/amd64
if [ ${#Images[@]} -eq 1 ]; then 
    if [ ${#Archs[@]}  -eq 0 ]; then
		Archs=("linux/amd64")
	fi
fi

if [ ${#Images[@]} -ne ${#Archs[@]} ]; then 
	echo "arch and image should one to one"
	usage && exit $ERRUSE
fi

function push()
{
	image="$1"
	arch="$2"
	user="$3"
	
	echo "push command $user --platform $arch -k $image "
	$CTR i push $user --platform $arch -k $image
}

trap 'delete' EXIT

function delete()
{
    for i in "${!Images[@]}"; do
		echo "delete image tag ${Images[$i]}"
        $CTR i rm ${Images[$i]}
    done
}

function main()
{
	$CTR i pull --all-platforms $Image
	for i in "${!Images[@]}"; do
		echo "add image tag $img"
        $CTR i tag $Image ${Images[$i]}
		echo "img ${Images[$i]}, arch ${Archs[$i]}, user $UserPw"
		push ${Images[$i]} ${Archs[$i]} "$UserPw"
    done
}
main