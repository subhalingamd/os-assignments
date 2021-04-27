submission_folder="2018MT10770_A4"
mkdir ${submission_folder}

for file in mysemaphore.h rwlock.h rwlock-reader-pref.c rwlock-writer-pref.c
	do cp assignment_code/${file} ${submission_folder}
done

zip -r ${submission_folder}.zip ${submission_folder}