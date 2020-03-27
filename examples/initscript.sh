#!/usr/bin/cgctl --options=cpu_usage=50,mem_usage=25

prog="Some daemon"
PIDFILE=/var/run/program.pid

start()
{
	echo $"Starting $prog:"
	program --daemon --pidfile=$PIDFILE
}

stop()
{
	echo $"Stopping $prog:"
	kill -TERM $(<$PIDFILE)
}

status()
{
	kill -0 $(<$PIDFILE) && echo "Working." || echo "Dead."
}

RETVAL=1

case "$1" in
	start)
		start
		RETVAL=$?
		;;
	stop)
		stop
		RETVAL=$?
		;;
	restart)
		stop
		start
		RETVAL=$?
		;;
	status)
		status		
		RETVAL=$?
		;;
	*)
		echo "Unknown action $1" 1>&2
		;;
esac

exit $RETVAL
