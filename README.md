tty-pomodoro
============

A terminal based pomodoro timer

Current status: in development

TODO:

* remove unnecessary calculations (old clock code)
* add option for short (5 minute) and long (10 minute) breaks
* fix issue with 59 and 60 seconds not showing
* add a notification or sound at the end of time
* remove all old references to tty-clock

usage : tty-pomodoro [-ivsScbtrahDBxn] [-C [0-7]] [-d delay] [-a nsdelay] [-T tty] 
    -S            Screensaver mode                               
    -x            Show box                                       
    -c            Set the timer at the center of the terminal    
    -C [0-7]      Set the timer color                            
    -b            Use bold colors                                
    -T tty        Display the timer on the specified terminal    
    -r            Do rebound the timer                           
    -n            Don't quit on keypress                         
    -v            Show tty-pomodoro version                         
    -i            Show some info about tty-pomodoro
    -h            Show this page                                 
    -B            Enable blinking colon                          
    -d delay      Set the delay between two redraws of the timer. Default 1s. 
    -a nsdelay    Additional delay between two redraws in nanoseconds. Default 0ns.