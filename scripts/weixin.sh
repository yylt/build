#!/bin/bash

groupmod -o -g $AUDIO_GID audio
groupmod -o -g $VIDEO_GID video
if [ $GID != $(echo `id -g wechat`) ]; then
    groupmod -o -g $GID wechat
fi
if [ $UID != $(echo `id -u wechat`) ]; then
    usermod -o -u $UID wechat
fi
chown wechat:wechat /WeChatFiles

# APP 支持 weixin 和 weixin.work
su wechat <<EOF
    echo "启动 $APP"
    "/opt/apps/com.qq.$APP.deepin/files/run.sh"
   sleep 300
EOF

while test -n "`pidof WeChat.exe`"
do
    sleep 60
done
echo "退出"