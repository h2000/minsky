export LD_LIBRARY_PATH=$HOME/usr/lib:/usr/local/lib:$LD_LIBRARY_PATH
EXIT=0

here=`pwd`
mkdir /tmp/$$
cd /tmp/$$

for i in $here/examples/*.mky; do
  "$here/minsky" "$here/test/rewriteMky.tcl" "$i"  tmp;
  diff -q -w "$i" tmp
  if [ $? -ne 0 ]; then
      echo "$i mutates"
      EXIT=1
  fi
done

rm -rf /tmp/$$
exit $EXIT
