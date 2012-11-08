export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
if [ -x test/unittests ]; then
    test/unittests
else
    exit 1;
fi
