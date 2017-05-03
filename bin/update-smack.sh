git checkout share/smack/svcomp/utils.py
sed -i -- "s/recursionBound:65536/recursionBound:$1/g" share/smack/svcomp/utils.py
sed -i -- "s/args\.unroll = 100/args\.unroll = $1/g" share/smack/svcomp/utils.py

sed -i -- "s/INSTALL_DEPENDENCIES=1/INSTALL_DEPENDENCIES=0/g" bin/build.sh
sed -i -- "s/BUILD_Z3=1/BUILD_Z3=0/g" bin/build.sh
sed -i -- "s/BUILD_BOOGIE=1/BUILD_BOOGIE=0/g" bin/build.sh
sed -i -- "s/BUILD_CORRAL=1/BUILD_CORRAL=0/g" bin/build.sh
sed -i -- "s/BUILD_LOCKPWN=1/BUILD_LOCKPWN=0/g" bin/build.sh

sudo bin/build.sh
