git checkout share/smack/svcomp/utils.py
sed -i -- "s/recursionBound:65536/recursionBound:$1/g" share/smack/svcomp/utils.py
sed -i -- "s/args\.unroll = 100/args\.unroll = $1/g" share/smack/svcomp/utils.py
