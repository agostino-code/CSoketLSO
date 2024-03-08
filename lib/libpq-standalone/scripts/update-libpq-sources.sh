#! /bin/sh
set -e

task="update"

function xcp()
{
    if [ $task = "update" ] ; then
        if ! [ -f $2 ] || ! cmp $1 $2 ; then
            echo "Updating $2"
            cp $1 $2
        fi
    elif [ $task = "clear" ] ; then
        echo "Removing $2"
        rm -f $2
    fi
}


if [ "$1" = "clear" ] ; then
    task="clear"
fi

pg_src=${HOME}/src/postgresql
dst_dir=$(dirname $0)/../libpq
dst_dir=$(realpath $dst_dir)

echo "Copying from $pg_src -> $dst_dir"

mkdir -p $dst_dir/common
mkdir -p $dst_dir/include
mkdir -p $dst_dir/libpq
mkdir -p $dst_dir/mb
mkdir -p $dst_dir/port
mkdir -p $dst_dir/win32_msvc
mkdir -p $dst_dir/win32_msvc/sys
mkdir -p $dst_dir/win32
mkdir -p $dst_dir/win32/arpa
mkdir -p $dst_dir/win32/netinet
mkdir -p $dst_dir/win32/sys

libpq_src="fe-auth-scram.c
    fe-connect.c
    fe-exec.c
    fe-lobj.c
    fe-misc.c
    fe-print.c
    fe-protocol3.c
    fe-secure.c
    fe-secure-common.c fe-secure-common.h
    fe-secure-openssl.c
    fe-trace.c
    legacy-pqsignal.c
    libpq-events.c
    libpq-int.h
    pqexpbuffer.c pqexpbuffer.h
    pthread-win32.c
    fe-auth.c fe-auth.h
    fe-auth-sasl.h
    win32.c win32.h"
    
common_src="
    base64.c
    cryptohash.c
    encnames.c
    hmac.c
    ip.c
    link-canary.c
    md5.c
    md5_common.c
    md5_int.h
    saslprep.c
    scram-common.c
    sha1.c
    sha1_int.h
    sha2.c
    sha2_int.h
    string.c
    unicode_norm.c
    wchar.c
"


port_src="
    chklocale.c
    getaddrinfo.c
    getpeereid.c
    dirmod.c
    explicit_bzero.c
    inet_aton.c
    inet_net_ntop.c
    noblock.c
    open.c
    pgsleep.c
    pgstrcasecmp.c
    snprintf.c
    strerror.c
    strlcpy.c
    pg_strong_random.c
    pthread-win32.h
    thread.c
    win32error.c
    win32setlocale.c
"

for f in $libpq_src ; do
    xcp $pg_src/src/interfaces/libpq/$f $dst_dir/$f
done

for f in $common_src ; do
    xcp $pg_src/src/common/$f $dst_dir/$f
done

for f in $port_src ; do
    xcp $pg_src/src/port/$f $dst_dir/$f
done

libpq_priv_include="postgres_fe.h
    port.h
    c.h
    common/base64.h
    common/config_info.h
    common/cryptohash.h
    common/fe_memutils.h
    common/hmac.h
    common/ip.h
    common/link-canary.h
    common/md5.h
    common/openssl.h
    common/saslprep.h
    common/scram-common.h
    common/sha1.h
    common/sha2.h
    common/string.h
    common/unicode_combining_table.h
    common/unicode_norm.h
    common/unicode_norm_table.h
    common/unicode_east_asian_fw_table.h
    mb/pg_wchar.h
    port/pg_bswap.h
    port/pg_crc32c.h
    port/win32_port.h
    "


for f in $libpq_priv_include ; do
    xcp $pg_src/src/include/$f $dst_dir/$f
done


libpq_pub_include="postgres_ext.h
    getaddrinfo.h
    pg_config_manual.h"

for f in $libpq_pub_include ; do
    xcp $pg_src/src/include/$f $dst_dir/include/$f
done

libpq_pub_libpq_include="libpq-fe.h
    libpq-events.h
"

for f in $libpq_pub_libpq_include ; do
    xcp $pg_src/src/interfaces/libpq/$f $dst_dir/include/$f
done

# xcp $pg_src/src/backend/utils/mb/wchar.c $dst_dir/wchar.c
# xcp $pg_src/src/backend/utils/mb/encnames.c $dst_dir/encnames.c
xcp $pg_src/src/include/libpq/pqcomm.h $dst_dir/libpq/pqcomm.h
xcp $pg_src/src/include/libpq/libpq-fs.h $dst_dir/libpq/libpq-fs.h

xcp $pg_src/src/include/port/win32/arpa/inet.h $dst_dir/win32/arpa/inet.h
xcp $pg_src/src/include/port/win32/netdb.h $dst_dir/win32/netdb.h
xcp $pg_src/src/include/port/win32/netinet/in.h $dst_dir/win32/netinet/in.h
xcp $pg_src/src/include/port/win32/pwd.h $dst_dir/win32/pwd.h
xcp $pg_src/src/include/port/win32_msvc/sys/file.h $dst_dir/win32/sys/file.h
xcp $pg_src/src/include/port/win32_msvc/sys/param.h $dst_dir/win32/sys/param.h
xcp $pg_src/src/include/port/win32/sys/socket.h $dst_dir/win32/sys/socket.h
xcp $pg_src/src/include/port/win32_msvc/sys/time.h $dst_dir/win32_msvc/sys/time.h
xcp $pg_src/src/include/port/win32_msvc/unistd.h $dst_dir/win32_msvc/unistd.h

# FIXME: adapt DEFAULT_PGSOCKET_DIR

# FIXME: genrate instead of copy
#xcp $pg_src/build/src/include/pg_config_ext.h $dst_dir/include/pg_config_ext.h
#xcp $pg_src/build/src/include/pg_config_os.h $dst_dir/include/pg_config_os.h
#xcp $pg_src/build/src/port/pg_config_paths.h $dst_dir/include/pg_config_paths.h
