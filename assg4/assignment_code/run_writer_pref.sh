gcc test-writer-pref.c rwlock-writer-pref.c -o rwlock-writer-pref -lpthread

./rwlock-writer-pref 10 4

#clean
rm ./rwlock-writer-pref
