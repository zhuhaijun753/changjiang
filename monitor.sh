#!/bin/sh

set -e

# here user can indicate the user app path
AppProgram=/data/tbox
NewProgram=/data/LteUpgrade.bin
BackupProgram=/data/backupTbox
LOG=/data/log.txt


check_process()
{
	echo " check_process!" >>$LOG
	
	count=`ps -ef |grep tbox |grep -v "grep" |wc -l`
	echo -e " count = $count \n" >>$LOG
	
	if [ $count -eq 0 ];then
		echo -e " Running new programme error, back to old version!" >>$LOG
		rm -rf $AppProgram
		mv $BackupProgram $AppProgram
		$AppProgram &
	else
		echo " Running new programme successfully!" >>$LOG
	fi
}


case "$1" in
	start)

		echo -n ">>> Starting tbox application: " >$LOG
		if [ "$2" -eq "0" ]; then
		
			#if ! [ -f $AppProgram ]; then
			#	echo "### tbox Application doesn't exist ###" >>$LOG
			#	exit 3
			#fi
		
			if [ -e $NewProgram ]; then
				echo " Exist LTE upgrade file: $NewProgram!" >>$LOG
				
				#if [ -e $BackupProgram ]; then
				#	echo -e " Delete old version $BackupProgram!\n" >>$LOG
				#	rm -rf $BackupProgram
				#fi
				
				if [ ! -x $NewProgram ];then
					chmod 777 $NewProgram
					echo " Assigning $NewProgram authority!" >>$LOG
				fi
			
				if [ -e $AppProgram ];then
					echo " Backup tbox and runing New tbox!" >>$LOG
					mv $AppProgram $BackupProgram
				fi
				mv $NewProgram $AppProgram
				$AppProgram &
			else
				echo " run tbox programme!!!" >>$LOG
				if [ ! -x $AppProgram ];then
					chmod 777 $AppProgram
				fi
				$AppProgram &
			fi
		else
			echo -e " Running new programme error, back to old version!\n" >>$LOG
			if [ -f $BackupProgram ]; then
				if [ -f $AppProgram ]; then
					echo -e "### delete the tbox which can not runing.\n" >>$LOG
					rm -rf $AppProgram
				fi
				echo -e " Running old tbox version!\n" >>$LOG
				mv $BackupProgram $AppProgram
			fi
			echo -e " Running tbox!\n" >>$LOG
			if [ ! -x $AppProgram ];then
				chmod 777 $AppProgram
			fi
			$AppProgram &
		fi
		;;
	stop)
		echo -n "Stopping quectel daemon: "
		start-stop-daemon -K -n $AppProgram
		echo "done"
		;;
	restart)
		$0 stop
		$0 start
		;;
	*)
		echo "Usage: tbox { start | stop | restart }" >&2
		exit 1
		;;
esac

exit 0
