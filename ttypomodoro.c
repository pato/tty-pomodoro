/*
 *      TTY-CLOCK Main file.
 *      Copyright © 2008 Martin Duquesnoy <xorg62@gmail.com>
 *      All rights reserved.
 *
 *      Redistribution and use in source and binary forms, with or without
 *      modification, are permitted provided that the following conditions are
 *      met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following disclaimer
 *        in the documentation and/or other materials provided with the
 *        distribution.
 *      * Neither the name of the  nor the names of its
 *        contributors may be used to endorse or promote products derived from
 *        this software without specific prior written permission.
 *
 *      THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *      "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *      LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *      A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *      OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *      SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *      LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *      DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *      THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *      (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *      OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ttypomodoro.h"

static time_t start_time;
static unsigned int start_minutes;

void init(void){
     struct sigaction sig;
     ttyclock->bg = COLOR_BLACK;

     /* Init ncurses */
     if (ttyclock->tty) {
	     FILE *ftty = fopen(ttyclock->tty, "r+");
	     if (!ftty) {
		     fprintf(stderr, "tty-clock: error: '%s' couldn't be opened: %s.\n",
				     ttyclock->tty, strerror(errno));
		     exit(EXIT_FAILURE);
	     }
	     ttyclock->ttyscr = newterm(NULL, ftty, ftty);
	     assert(ttyclock->ttyscr != NULL);
	     set_term(ttyclock->ttyscr);
     } else
	     initscr();

     cbreak();
     noecho();
     keypad(stdscr, True);
     start_color();
     curs_set(False);
     clear();

     /* Init default terminal color */
     if(use_default_colors() == OK)
          ttyclock->bg = -1;

     /* Init color pair */
     init_pair(0, ttyclock->bg, ttyclock->bg);
     init_pair(1, ttyclock->bg, ttyclock->option.color);
     init_pair(2, ttyclock->option.color, ttyclock->bg);
     refresh();

     /* Init signal handler */
     sig.sa_handler = signal_handler;
     sig.sa_flags   = 0;
     sigaction(SIGWINCH, &sig, NULL);
     sigaction(SIGTERM,  &sig, NULL);
     sigaction(SIGINT,   &sig, NULL);
     sigaction(SIGSEGV,  &sig, NULL);

     /* Init global struct */
     ttyclock->running = True;
     if(!ttyclock->geo.x)
          ttyclock->geo.x = 0;
     if(!ttyclock->geo.y)
          ttyclock->geo.y = 0;
     if(!ttyclock->geo.a)
          ttyclock->geo.a = 1;
     if(!ttyclock->geo.b)
          ttyclock->geo.b = 1;
     ttyclock->geo.w = (ttyclock->option.second) ? SECFRAMEW : NORMFRAMEW;
     ttyclock->geo.h = 7;
     ttyclock->tm = localtime(&(ttyclock->lt));
     if(ttyclock->option.utc) {
         ttyclock->tm = gmtime(&(ttyclock->lt));
     }
     ttyclock->lt = time(NULL);
     update_hour();

     /* Create clock win */
     ttyclock->framewin = newwin(ttyclock->geo.h,
                                 ttyclock->geo.w,
                                 ttyclock->geo.x,
                                 ttyclock->geo.y);
     if(ttyclock->option.box) {
           box(ttyclock->framewin, 0, 0);
     }

     if (ttyclock->option.bold)
     {
          wattron(ttyclock->framewin, A_BLINK);
     }

     /* Create the date win */
     ttyclock->datewin = newwin(DATEWINH, strlen(ttyclock->date.datestr) + 2,
                                ttyclock->geo.x + ttyclock->geo.h - 1,
                                ttyclock->geo.y + (ttyclock->geo.w / 2) -
                                (strlen(ttyclock->date.datestr) / 2) - 1);
     if(ttyclock->option.box) {
          box(ttyclock->datewin, 0, 0);
     }
     clearok(ttyclock->datewin, True);

     set_center(ttyclock->option.center);

     nodelay(stdscr, True);

     if (ttyclock->option.date)
     {
          wrefresh(ttyclock->datewin);
     }

     wrefresh(ttyclock->framewin);

     /* Initialize the start timer */
     start_time = time(0);

     return;
}

void signal_handler(int signal){
     switch(signal)
     {
     case SIGWINCH:
          endwin();
          init();
          break;
          /* Interruption signal */
     case SIGINT:
     case SIGTERM:
          ttyclock->running = False;
          /* Segmentation fault signal */
          break;
     case SIGSEGV:
          endwin();
          fprintf(stderr, "Segmentation fault.\n");
          exit(EXIT_FAILURE);
          break;
     }

     return;
}

void cleanup(void){
	if (ttyclock->ttyscr)
		delscreen(ttyclock->ttyscr);

	if (ttyclock && ttyclock->tty)
		free(ttyclock->tty);
	if (ttyclock && ttyclock->option.format)
		free(ttyclock->option.format);
	if (ttyclock)
		free(ttyclock);
}

void update_hour(void){
     /* Static minute and hour counts so future calls can read previous values */
     static unsigned int c_minute = 0, c_second = 0;

     /* Get current time */
     time_t curr_time = time(0);

     /* Calculate the time remaining */
     unsigned int time_diff = curr_time - start_time;
     time_diff = start_minutes*60 - time_diff;

     /* Calculate seconds and minutes remaining */
     c_second = time_diff - (c_minute*60);
     c_second = c_second < 60 ? c_second : 0;
     c_minute = time_diff/60;

     if (c_minute == 0 && c_second == 0){
        time_ended();
     }
    
     /* Set hour */
     ttyclock->date.hour[0] = c_minute / 10;
     ttyclock->date.hour[1] = c_minute % 10;

     /* Set minutes */
     ttyclock->date.minute[0] = c_second / 10;
     ttyclock->date.minute[1] = c_second % 10;

     return;
}

void time_ended(){
    printf("Time ended!\n");
    exit(EXIT_SUCCESS);
    return;
}

void draw_number(int n, int x, int y){
     int i, sy = y;

     for(i = 0; i < 30; ++i, ++sy)
     {
          if(sy == y + 6)
          {
               sy = y;
               ++x;
          }

          if (ttyclock->option.bold)
               wattron(ttyclock->framewin, A_BLINK);
          else
               wattroff(ttyclock->framewin, A_BLINK);

          wbkgdset(ttyclock->framewin, COLOR_PAIR(number[n][i/2]));
          mvwaddch(ttyclock->framewin, x, sy, ' ');
     }
     wrefresh(ttyclock->framewin);

     return;
}

void draw_clock(void){
     /* Draw hour numbers */
     draw_number(ttyclock->date.hour[0], 1, 1);
     draw_number(ttyclock->date.hour[1], 1, 8);

     if (ttyclock->option.blink){
       time_t seconds;
       seconds = time(NULL);

       if (seconds % 2 != 0){
           /* 2 dot for number separation */
           wbkgdset(ttyclock->framewin, COLOR_PAIR(1));
           mvwaddstr(ttyclock->framewin, 2, 16, "  ");
           mvwaddstr(ttyclock->framewin, 4, 16, "  ");
       }
       else if (seconds % 2 == 0){
           /*2 dot black for blinking */
           wbkgdset(ttyclock->framewin, COLOR_PAIR(2));
           mvwaddstr(ttyclock->framewin, 2, 16, "  ");
           mvwaddstr(ttyclock->framewin, 4, 16, "  ");
       }
     }
     else{
       /* 2 dot for number separation */
       wbkgdset(ttyclock->framewin, COLOR_PAIR(1));
       mvwaddstr(ttyclock->framewin, 2, 16, "  ");
       mvwaddstr(ttyclock->framewin, 4, 16, "  ");
     }

     /* Draw minute numbers */
     draw_number(ttyclock->date.minute[0], 1, 20);
     draw_number(ttyclock->date.minute[1], 1, 27);

     /* Draw the date */
     if (ttyclock->option.bold)
          wattron(ttyclock->datewin, A_BOLD);
     else
          wattroff(ttyclock->datewin, A_BOLD);

     if (ttyclock->option.date)
     {
          wbkgdset(ttyclock->datewin, (COLOR_PAIR(2)));
          mvwprintw(ttyclock->datewin, (DATEWINH / 2), 1, ttyclock->date.datestr);
          wrefresh(ttyclock->datewin);
     }

     /* Draw second if the option is enable */
     if(ttyclock->option.second)
     {
          /* Again 2 dot for number separation */
          wbkgdset(ttyclock->framewin, COLOR_PAIR(1));
          mvwaddstr(ttyclock->framewin, 2, NORMFRAMEW, "  ");
          mvwaddstr(ttyclock->framewin, 4, NORMFRAMEW, "  ");

          /* Draw second numbers */
          draw_number(ttyclock->date.second[0], 1, 39);
          draw_number(ttyclock->date.second[1], 1, 46);
     }

     return;
}

void clock_move(int x, int y, int w, int h){

     /* Erase border for a clean move */
     wbkgdset(ttyclock->framewin, COLOR_PAIR(0));
     wborder(ttyclock->framewin, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
     werase(ttyclock->framewin);
     wrefresh(ttyclock->framewin);

     if (ttyclock->option.date)
     {
          wbkgdset(ttyclock->datewin, COLOR_PAIR(0));
          wborder(ttyclock->datewin, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
          werase(ttyclock->datewin);
          wrefresh(ttyclock->datewin);
     }

     /* Frame win move */
     mvwin(ttyclock->framewin, (ttyclock->geo.x = x), (ttyclock->geo.y = y));
     wresize(ttyclock->framewin, (ttyclock->geo.h = h), (ttyclock->geo.w = w));

     /* Date win move */
     if (ttyclock->option.date)
     {
          mvwin(ttyclock->datewin,
                ttyclock->geo.x + ttyclock->geo.h - 1,
                ttyclock->geo.y + (ttyclock->geo.w / 2) - (strlen(ttyclock->date.datestr) / 2) - 1);
          wresize(ttyclock->datewin, DATEWINH, strlen(ttyclock->date.datestr) + 2);

          if (ttyclock->option.box) {
            box(ttyclock->datewin,  0, 0);
          }
     }

     if (ttyclock->option.box)
     {
        box(ttyclock->framewin, 0, 0);
     }

     wrefresh(ttyclock->framewin);
     wrefresh(ttyclock->datewin); 
     return;
}

/* Useless but fun :) */
void clock_rebound(void){
     if(!ttyclock->option.rebound)
          return;

     if(ttyclock->geo.x < 1)
          ttyclock->geo.a = 1;
     if(ttyclock->geo.x > (LINES - ttyclock->geo.h - DATEWINH))
          ttyclock->geo.a = -1;
     if(ttyclock->geo.y < 1)
          ttyclock->geo.b = 1;
     if(ttyclock->geo.y > (COLS - ttyclock->geo.w - 1))
          ttyclock->geo.b = -1;

     clock_move(ttyclock->geo.x + ttyclock->geo.a,
                ttyclock->geo.y + ttyclock->geo.b,
                ttyclock->geo.w,
                ttyclock->geo.h);

     return;
}

void set_second(void){
     int new_w = (((ttyclock->option.second = !ttyclock->option.second)) ? SECFRAMEW : NORMFRAMEW);
     int y_adj;

     for(y_adj = 0; (ttyclock->geo.y - y_adj) > (COLS - new_w - 1); ++y_adj);

     clock_move(ttyclock->geo.x, (ttyclock->geo.y - y_adj), new_w, ttyclock->geo.h);

     set_center(ttyclock->option.center);

     return;
}

void set_center(Bool b){
     if((ttyclock->option.center = b))
     {
          ttyclock->option.rebound = False;

          clock_move((LINES / 2 - (ttyclock->geo.h / 2)),
                     (COLS  / 2 - (ttyclock->geo.w / 2)),
                     ttyclock->geo.w,
                     ttyclock->geo.h);
     }

     return;
}

void set_box(Bool b){
     ttyclock->option.box = b;

     wbkgdset(ttyclock->framewin, COLOR_PAIR(0));
     wbkgdset(ttyclock->datewin, COLOR_PAIR(0));

     if(ttyclock->option.box) {
         wbkgdset(ttyclock->framewin, COLOR_PAIR(0));
         wbkgdset(ttyclock->datewin, COLOR_PAIR(0));
         box(ttyclock->framewin, 0, 0);
         box(ttyclock->datewin,  0, 0);
     }
     else {
         wborder(ttyclock->framewin, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
         wborder(ttyclock->datewin, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
     }

     wrefresh(ttyclock->datewin);
     wrefresh(ttyclock->framewin);
}

void key_event(void){
     int i, c;

     struct timespec length = { ttyclock->option.delay, ttyclock->option.nsdelay };
     
     if (ttyclock->option.screensaver)
     {
          c = wgetch(stdscr);
          if(c != ERR && ttyclock->option.noquit == False)
          {
               ttyclock->running = False;
          }
          else
          {
               nanosleep(&length, NULL);
               for(i = 0; i < 8; ++i)
                    if(c == (i + '0'))
                    {
                         ttyclock->option.color = i;
                         init_pair(1, ttyclock->bg, i);
                         init_pair(2, i, ttyclock->bg);
                    }
          }
          return;
     }
     

     switch(c = wgetch(stdscr))
     {
     case KEY_UP:
     case 'k':
     case 'K':
          if(ttyclock->geo.x >= 1
             && !ttyclock->option.center)
               clock_move(ttyclock->geo.x - 1, ttyclock->geo.y, ttyclock->geo.w, ttyclock->geo.h);
          break;

     case KEY_DOWN:
     case 'j':
     case 'J':
          if(ttyclock->geo.x <= (LINES - ttyclock->geo.h - DATEWINH)
             && !ttyclock->option.center)
               clock_move(ttyclock->geo.x + 1, ttyclock->geo.y, ttyclock->geo.w, ttyclock->geo.h);
          break;

     case KEY_LEFT:
     case 'h':
     case 'H':
          if(ttyclock->geo.y >= 1
             && !ttyclock->option.center)
               clock_move(ttyclock->geo.x, ttyclock->geo.y - 1, ttyclock->geo.w, ttyclock->geo.h);
          break;

     case KEY_RIGHT:
     case 'l':
     case 'L':
          if(ttyclock->geo.y <= (COLS - ttyclock->geo.w - 1)
             && !ttyclock->option.center)
               clock_move(ttyclock->geo.x, ttyclock->geo.y + 1, ttyclock->geo.w, ttyclock->geo.h);
          break;

     case 'q':
     case 'Q':
          if (ttyclock->option.noquit == False)
		  ttyclock->running = False;
          break;

     case 's':
     case 'S':
          set_second();
          break;

     case 't':
     case 'T':
          ttyclock->option.twelve = !ttyclock->option.twelve;
          /* Set the new ttyclock->date.datestr to resize date window */
          update_hour();
          clock_move(ttyclock->geo.x, ttyclock->geo.y, ttyclock->geo.w, ttyclock->geo.h);
          break;

     case 'c':
     case 'C':
          set_center(!ttyclock->option.center);
          break;

     case 'b':
     case 'B':
          ttyclock->option.bold = !ttyclock->option.bold;
          break;

     case 'r':
     case 'R':
          ttyclock->option.rebound = !ttyclock->option.rebound;
          if(ttyclock->option.rebound && ttyclock->option.center)
               ttyclock->option.center = False;
          break;

     case 'x':
     case 'X':
          set_box(!ttyclock->option.box);
          break;

     default:
          nanosleep(&length, NULL);
          for(i = 0; i < 8; ++i)
               if(c == (i + '0'))
               {
                    ttyclock->option.color = i;
                    init_pair(1, ttyclock->bg, i);
                    init_pair(2, i, ttyclock->bg);
               }
          break;
     }

     return;
}

void print_usage(){
   printf("usage : tty-clock [-ivcbrahBxn] [-C [0-7]] [-d delay] [-a nsdelay] [-T tty] [short | long]\n"
              "    -x            Show box                                       \n"
              "    -c            Set the timer at the center of the terminal    \n"
              "    -C [0-7]      Set the clock color                            \n"
              "    -b            Use bold colors                                \n"
              "    -T tty        Display the timer on the specified terminal    \n"
              "    -r            Do rebound the timer                           \n"
              "    -n            Don't quit on keypress                         \n"
              "    -v            Show tty-pomodoro version                      \n"
              "    -i            Show some info about tty-pomodoro              \n"
              "    -h            Show this page                                 \n"
              "    -B            Enable blinking colon                          \n"
              "    -d delay      Set the delay between two redraws of the timer . Default 1s. \n"
              "    -a nsdelay    Additional delay between two redraws in nanoseconds. Default 0ns.\n"
              "    short         Take a five minute break                       \n"
              "    long          Take a ten minute break                        \n");
}

int main(int argc, char **argv){
     int c;

     /* Alloc ttyclock */
     ttyclock = malloc(sizeof(ttyclock_t));
     assert(ttyclock != NULL);
     memset(ttyclock, 0, sizeof(ttyclock_t));

     ttyclock->option.date = True;

     /* Date format */
     ttyclock->option.format = malloc(sizeof(char) * 100);
     /* Default date format */
     strncpy(ttyclock->option.format, "%F", 100);
     /* Default color */
     ttyclock->option.color = COLOR_RED; 
     /* Default delay */
     ttyclock->option.delay = 1; /* 1FPS */
     ttyclock->option.nsdelay = 0; /* -0FPS */
     ttyclock->option.blink = False;

     /* Never show seconds */
     ttyclock->option.second = False;

     /* Hide the date */
     ttyclock->option.date = False;

     atexit(cleanup);

     while ((c = getopt(argc, argv, "ivcbrhBxnC:d:T:a:")) != -1){
          switch(c)
          {
          case 'h':
          default:
               print_usage();
               exit(EXIT_SUCCESS);
               break;
          case 'i':
               puts("TTY-Clock 2 © by Martin Duquesnoy (xorg62@gmail.com), Grey (grey@greytheory.net)");
               exit(EXIT_SUCCESS);
               break;
          case 'v':
               puts("TTY-Clock 2 © devel version");
               exit(EXIT_SUCCESS);
               break;
          case 'c':
               ttyclock->option.center = True;
               break;
          case 'b':
               ttyclock->option.bold = True;
               break;
          case 'C':
               if(atoi(optarg) >= 0 && atoi(optarg) < 8)
                    ttyclock->option.color = atoi(optarg);
               break;
          case 'r':
               ttyclock->option.rebound = True;
               break;
          case 'd':
               if(atol(optarg) >= 0 && atol(optarg) < 100)
                    ttyclock->option.delay = atol(optarg);
               break;
          case 'B':
               ttyclock->option.blink = True;
               break;
          case 'a':
               if(atol(optarg) >= 0 && atol(optarg) < 1000000000)
                    ttyclock->option.nsdelay = atol(optarg);
                break;
          case 'x':
               ttyclock->option.box = True;
               break;
	  case 'T': {
	       struct stat sbuf;
	       if (stat(optarg, &sbuf) == -1) {
		       fprintf(stderr, "tty-clock: error: couldn't stat '%s': %s.\n",
				       optarg, strerror(errno));
		       exit(EXIT_FAILURE);
	       } else if (!S_ISCHR(sbuf.st_mode)) {
		       fprintf(stderr, "tty-clock: error: '%s' doesn't appear to be a character device.\n",
				       optarg);
		       exit(EXIT_FAILURE);
	       } else {
	       		if (ttyclock->tty)
				free(ttyclock->tty);
			ttyclock->tty = strdup(optarg);
	       }}
	       break;
	  case 'n':
	       ttyclock->option.noquit = True;
	       break;
          }
     }

     /* Set the default minutes to 25 */
     start_minutes = DEFAULT_TIME;

     /* Check if short or long break */
     if (optind < argc){
        char *argument = argv[optind];
        if (!strcmp(argument, "short")){
            start_minutes = SHORT_BREAK;
        }else if (!strcmp(argument, "long")){
            start_minutes = LONG_BREAK;
        }else{
            printf("Command not recognized\n");
            print_usage();
            exit(EXIT_FAILURE);
        }
     }

     init();
     attron(A_BLINK);
     while(ttyclock->running){
          clock_rebound();
          update_hour();
          draw_clock();
          key_event();
     }

     endwin();

     return 0;
}
