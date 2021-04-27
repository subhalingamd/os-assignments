gcc test-reader-pref.c rwlock-reader-pref.c -o rwlock-reader-pref -lpthread

./rwlock-reader-pref 10 4

#clean
rm ./rwlock-reader-pref