#! /bin/bash
#
# Add the newly created photos to the Subversion repository
#

src=`cd \`dirname $0\`; pwd -P`
echo "svn up   --non-interactive  --trust-server-cert  $src" > /tmp/xxx
svn st $src|grep '^?' | grep '@' | awk '{print "svn add "$2"@" }' >> /tmp/xxx
echo "svn ci   --non-interactive  --trust-server-cert  -m '' $src" >> /tmp/xxx

. /tmp/xxx
rm -rf /tmp/xxx

