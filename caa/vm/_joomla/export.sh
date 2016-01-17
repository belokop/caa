#! /bin/bash
cd `dirname $0`
pkg=`basename \`pwd -P\``
echo $pkg
find ./ -name "*~" -exec rm -vf {} \;
rm -fr ~belokop/public_html/$pkg.zip
zip    ~belokop/public_html/$pkg *

cp -pvf ~belokop/public_html/$pkg.zip /tmp/joomla/tmp/

