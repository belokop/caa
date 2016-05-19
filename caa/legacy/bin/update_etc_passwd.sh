#! /bin/bash
#
# Script to update the /etc/password file 
#

##### Not used since the end of 2015
# repository=https://plushkin.googlecode.com/svn/trunk/setup/maintenance/mail/2leif

##### Not used since the spring 2016
# repository=https://svn.csc.kth.se/~iourib/setup/maintenance/mail/2leif

# In production since 2016-04-01
repository=https://svn.csc.kth.se/~iourib/setup/registration

function usage(){
    msg="$1"
    [ -n "$msg" ] && {
	echo  error:  >&2
	echo "error: $msg" >&2
	echo  error:  >&2
    } || {
	echo usage: $0 "<user name> <first name> <last name>" >&2
    }
    exit 1
}

export PATH=/usr/pkg/subversion/pro/bin:$PATH
#
# Get arguments
#
user=$1
firstname=$2
lastname=$3
[ -n "$lastname" ] || usage

#
# Get the home directory
#
l1=`echo $user|cut -c1`
l2=`echo $user|cut -c2`
[ -z "$l1" ] && usage
[ -z "$l2" ] && usage
home=/afs/kth.se/home/$l1/$l2/$user
[ -d $home ]   || { home=/home/$user; }
[ -d $home ]   || { usage "the expected home_dir '$home' does not exist"; }

#
# Get the latest passwd file from the repository
#
[ -d /data ] && tmp=/data/.2leif || tmp=/tmp/.2leif
tmp_passwd=$tmp/`basename $repository`/passwd

if [ -s $tmp_passwd ]; then
    [ -w $tmp_passwd ] ||  usage "You don\'t have enougth priviliges to add user" 
    cd `dirname $tmp_passwd`
    svn up --config-dir $tmp/.subversion --trust-server-cert --non-interactive >/dev/null
else
    rm -rf $tmp
    mkdir -p $tmp
    cd $tmp
    
    which svn>/dev/null || usage "can't find  'svn' command"
    svn co --config-dir $tmp/.subversion --trust-server-cert --non-interactive $repository >/dev/null
fi

id $user >/dev/null 2>&1 && {
    echo ... User already known, `id $user`  >&2
    echo -n "... "
    grep ^$user:.*$home: /etc/passwd
    echo
    exit 1
}

rm -rf /tmp/t
grep ^$user:.*$home $tmp_passwd > /tmp/t && {
    echo  >&2
    echo ... User $firstname $lastname is already registered, be patient >&2
    echo -n "    " >&2
    cat /tmp/t >&2
    echo >&2
    rm -rf /tmp/t
    exit 1
}

#
# Get the uid by reading the home directory
#
uid=`ls -ld $home|awk '{print $3}'`
[ -z "$uid" ] && usage

#
# Update and save the password file
#
line="$user:x:$uid:2000001:$firstname $lastname:$home:/bin/bash"
echo "$line" >> $tmp_passwd

cd `dirname $tmp_passwd`
svn ci --config-dir $tmp/.subversion --trust-server-cert --non-interactive -m "Adding user $firstname $lastname" $tmp_passwd
