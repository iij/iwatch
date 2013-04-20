/*
 * Copyright (c) 2000,2001 Internet Initiative Japan Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistribution with functional modification must include
 *    prominent notice stating how and when and by whom it is
 *    modified.
 *
 * THIS SOFTWARE IS PROVIDED BY ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.
 */

/*
 * HISTORY:
 *
 * This command is slightly derived from `watch' command come with
 * BSD/OS 3.1 by BSDI, Inc., which originally came from some free
 * distribution.  However it is thoroughly expaneded and rewrote, and
 * very few code segment is retained now.
 *
 * 2000-06-23, Takuya Sato @ IIJ
 *	Original expantion is done by Takuya Sato of IIJ to implement
 *	reverse option and scroll capabilty.
 *
 * 2000-06-26, Kazumasa Utashiro @ IIJ
 *	All source code was overhauled and implemented many
 *	interactive commands enabling various control from keyboard.
 *	Help screen was also implemented.
 *
 * 2001-12-17, Kazumasa Utashiro @ IIJ
 *	Added line-reverse-mode.
 */

/*
 * BUGS:
 *
 * o Buffer length check is not done in untabify() function.  This must be
 *   repaired asap.
 *
 */

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/errno.h>
#include <errno.h>

#define DEFAULT_INTERVAL 2
#define MAXLINE 300
#define MAXCOLUMN 180
#define MAX_COMMAND_LENGTH 128

typedef enum {
    REVERSE_NONE,
    REVERSE_CHAR,
    REVERSE_WORD,
    REVERSE_LINE
} reverse_mode_t;

typedef enum {
    RSLT_UPDATE,
    RSLT_REDRAW,
    RSLT_NOTOUCH,
    RSLT_ERROR
} kbd_result_t;

/*
 * Global symbols
 */
int opt_interval = DEFAULT_INTERVAL;	/* interval */
int opt_debug = 0;			/* debug option */
reverse_mode_t
    reverse_mode = REVERSE_NONE,	/* reverse mode */
    last_reverse_mode = REVERSE_CHAR;	/* remember previous reverse mode */
int start_line = 0, start_column = 0;	/* display offset coordinates */
int prefix = 0;				/* command prefix argument */
int pause_status = 0;			/* pause status */
time_t lastupdate;			/* last updated time */

char execute[MAX_COMMAND_LENGTH];
typedef char BUFFER[MAXLINE][MAXCOLUMN + 1];

#define max(x, y)	((x) > (y) ? (x) : (y))
#define min(x, y)	((x) < (y) ? (x) : (y))

#define ctrl(c)		((c) & 037)
int           main (int, char *[]);
void          command_loop (void);
int           display (BUFFER *, BUFFER *, reverse_mode_t);
void          read_result (BUFFER *);
kbd_result_t  kbd_command (int);
void          showhelp (void);
void          untabify (char *, int);
void          die (int);
void          usage (char *, const char *);

int main(argc, argv)
    int  argc;
    char *argv[];
{
    int ch;
    char *myname = argv[0];
    const char *optstr;

    /*
     * Command line option handling
     */
    optstr = "i:rewps:c:d";
    while ((ch = getopt(argc, argv, optstr)) != -1)
	switch (ch) {
	case 'i':
	    opt_interval = atoi(optarg);
	    if (opt_interval == 0 || argc < 3) {
		usage(myname, optstr);
		exit(1);
	    }
	    break;
	case 'r':
	    reverse_mode = REVERSE_CHAR;
	    break;
	case 'w':
	    reverse_mode = REVERSE_WORD;
	    break;
	case 'e':
	    reverse_mode = REVERSE_LINE;
	    break;
	case 'p':
	    pause_status = 1;
	    break;
	case 's':
	    start_line = atoi(optarg);
	    break;
	case 'c':
	    start_column = atoi(optarg);
	    break;
	case 'd':
	    opt_debug = 1;
	    break;
	default:
	    usage(myname, optstr);
	    exit(1);
	}
    argc -= optind;
    argv += optind;

    if (myname) {
	char *cp = rindex(myname, '/');
	if (cp)
	    myname = cp + 1;
    }

    /*
     * Build command string to give to popen
     */
    if (!*argv) {
	usage(myname, optstr);
	exit(1);
    }
    (void) bzero(execute, sizeof(execute));
    while (*argv) {
	if (strlen(execute) + 1 + strlen(*argv) + 1 > sizeof(execute)) {
	    fprintf(stderr, "Command name is too long\n");
	    exit(1);
	}

	if (*execute)
	    strlcat(execute, " ", sizeof(execute));
	strlcat(execute, *argv, sizeof(execute));
	argv++;
	argc--;
    }

    /*
     * Initialize signal
     */
    (void) signal(SIGINT, die);
    (void) signal(SIGTERM, die);
    (void) signal(SIGHUP, die);

    /*
     * Initialize curses environment
     */
    initscr();
    noecho();
    crmode();

    /*
     * Enter main processing loop and never come back here
     */
    command_loop();

    /* NOTREACHED */
    return 0;
}

void command_loop()
{
    int nfds;
    BUFFER buf0, buf1;
    fd_set readfds;
    struct timeval to;
    int i = 0;

    do {
	BUFFER *cur, *prev;

	if (i == 0) {
	    cur = prev = &buf0;
	} else if (i % 2  == 0) {
	    cur = &buf0; prev = &buf1;
	} else {
	    cur = &buf1; prev = &buf0;
	}

	read_result(cur);

    redraw:
	display(cur, prev, reverse_mode);

    input:
	to.tv_sec = opt_interval;
	to.tv_usec = 0;
	FD_ZERO(&readfds);
	FD_SET(fileno(stdin), &readfds);
   	nfds = select(1, &readfds, NULL, NULL, pause_status ? NULL : &to);
	if (nfds < 0)
	    switch (errno) {
	    case EINTR:
		goto input;
	    default:
		perror("select");
	    }
	else if (nfds > 0) {
	    int ch = getch();
	    kbd_result_t result = kbd_command(ch);

	    switch (result) {
	    case RSLT_UPDATE:	/* update buffer */
		break;
	    case RSLT_REDRAW:	/* scroll with current buffer */
		goto redraw;
	    case RSLT_NOTOUCH:	/* silently loop again */
		goto input;
	    case RSLT_ERROR:	/* error */
		fprintf(stderr, "\007");
		goto input;
	    }
	}
	i++;
    } while (1);
}

int display(cur, prev, reverse)
    BUFFER *cur, *prev;
    reverse_mode_t reverse;
{
    int screen_x, screen_y;
    int line /* , column */;
    char *ct;

    clear();
    clearok(stdscr, 0);

    move(0, 0);
    printw("\"%s\" ", execute);
    if (pause_status)
	printw("--PAUSE--");
    else
	printw("on every %d seconds", opt_interval);

    ct = ctime(&lastupdate);
    ct[24] = '\0';
    move(0, COLS - strlen(ct));
    addstr(ct);

#define MODELINE(HOTKEY,SWITCH,MODE) { \
	printw(HOTKEY); \
	if (reverse == SWITCH) standout(); \
	printw(MODE); \
	if (reverse == SWITCH) standend(); \
}

    move(1, COLS - 47);
    printw("Reverse mode:");
    MODELINE(" [w]", REVERSE_WORD, "word");
    MODELINE(" [e]", REVERSE_LINE, "line");
    MODELINE(" [r]", REVERSE_CHAR, "char");
    printw(" [t]toggle");

    move(1, 1);
    if (prefix)
	printw("%d ", prefix);
    if (start_line != 0 || start_column != 0)
	printw("(%d, %d)", start_line, start_column);

    if (!prev || (cur == prev))
	reverse = REVERSE_NONE;

    for (line = start_line, screen_y = 2;
	 screen_y < LINES && line < MAXLINE && (*cur)[line][0];
	 line++, screen_y++)
    {
	char *cur_line = (*cur)[line];
	char *p = cur_line + start_column;
	char *prev_line = (*prev)[line];
	char *pp = prev_line + start_column;

	screen_x = 0;
	move(screen_y, screen_x);

	while (*p && screen_x < COLS) {
	    if (reverse == REVERSE_NONE || (*p == *pp)) {
		addch(*p++); pp++;
		screen_x++;
		continue;
	    }

	    /*
	     * This method to reverse by word unit is not very fancy
	     * but it was easy to implement.  If you are urged to rewrite
	     * this algorithm, it is right thing and don't hesitate to do
	     * so!
	     */
	    /*
	     * If the word reverse option is specified and the current
	     * character is not a space, track back to the beginning
	     * of the word.
	     */
	    if (reverse == REVERSE_WORD && !isspace((int)*p)) {
		while (cur_line + start_column < p && !isspace((int)*(p - 1))) {
		    p--; pp--;
		    screen_x--;
		}
		move(screen_y, screen_x);
	    } else if (reverse == REVERSE_LINE) {
		p = cur_line + start_column;
		pp = prev_line + start_column;
		screen_x = 0;
		move(screen_y, screen_x);
	    }

	    standout();

	    /*
	     * Print character itself.
	     */
	    addch(*p++); pp++;
	    screen_x++;

	    /*
	     * If the word reverse option is specified, and the current
	     * character is not a space, print the whole word which
	     * includes current character.
	     */
	    if (reverse == REVERSE_WORD) {
		while (*p && !isspace((int)*p) && screen_x < COLS) {
		    addch(*p++); pp++;
		    screen_x++;
		}
	    }
	    else if (reverse == REVERSE_LINE) {
		while (screen_x < COLS) {
		    if (*p && *p != '\n') {
			addch(*p++); pp++;
		    } else {
			addch(' ');
		    }
		    screen_x++;
		}
	    }

	    standend();
	}
    }
    move(1, 0);
    refresh();
    return 1;
}

void read_result(buf)
    BUFFER *buf;
{
    FILE *fp;
    int i = 0;

    /*
     * Clear buffer
     */
    bzero(buf, sizeof(*buf));

    /*
     * Open pipe to command
     */
    if ((fp = popen(execute, "r")) == (FILE *)NULL) {
	perror("popen");
	exit(2);
    } 

    /*
     * Read command output and convert tab to spaces
     *
     * TODO: handle non-printable characters
     */
    while ((i < MAXLINE) && (fgets((*buf)[i], MAXCOLUMN, fp) != NULL)) {
	untabify((*buf)[i], sizeof((*buf)[i]));
	i++;
    }

    /*
     * Close it
     */
    pclose(fp);

    /*
     * Remember update time
     */
    time(&lastupdate);
}

kbd_result_t kbd_command(ch)
    int ch;		/* command character */
{
    switch (ch) {

    case '?':
	showhelp();
	refresh();
	return RSLT_REDRAW;

    case '\033':
	prefix = 0;
	return RSLT_REDRAW;

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
	prefix = prefix * 10 + (ch - '0');
	return RSLT_REDRAW;

	/*
	 * Update buffer
	 */
    case ' ':
	return RSLT_UPDATE;

	/*
	 * Pause switch
	 */
    case 'p':
	if ((pause_status = !pause_status) != 0)
	    return RSLT_REDRAW;
	else
	    return RSLT_UPDATE;

	/*
	 * Reverse control
	 */
    case 't':
	if (reverse_mode != REVERSE_NONE) {
	    last_reverse_mode = reverse_mode;
	    reverse_mode = REVERSE_NONE;
	}
	else {
	    reverse_mode = last_reverse_mode;
	}
	break;
    case 'r':
	reverse_mode = (reverse_mode == REVERSE_CHAR) ? REVERSE_NONE
						      : REVERSE_CHAR;
	break;
    case 'w':
	reverse_mode = (reverse_mode == REVERSE_WORD) ? REVERSE_NONE
						      : REVERSE_WORD;
	break;
    case 'e':
	reverse_mode = (reverse_mode == REVERSE_LINE) ? REVERSE_NONE
						      : REVERSE_LINE;
	break;

	/*
	 * Set interval
	 */
    case 'i':
	if (prefix > 0) {
	    opt_interval = prefix;
	    prefix = 0;
	}
	return RSLT_REDRAW;

	/*
	 * Vertical motion
	 */
    case '\n':
    case '+':
    case 'j':
	start_line = min(start_line + 1, MAXLINE - 1);
	break;
    case '-':
    case 'k':
	start_line = max(start_line - 1, 0);
	break;
    case 'd':
    case 'D':
    case ctrl('d'):
	start_line = min(start_line + ((LINES - 2) / 2), MAXLINE - 1);
	break;
    case 'u':
    case 'U':
    case ctrl('u'):
	start_line = max(start_line - ((LINES - 2) / 2), 0);
	break;
    case 'f':
    case ctrl('f'):
	start_line = min(start_line + (LINES - 2), MAXLINE - 1);
	break;
    case 'b':
    case ctrl('b'):
	start_line = max(start_line - (LINES - 2), 0);
	break;
    case 'g':
	if (prefix < MAXLINE)
	    start_line = prefix;
 	prefix = 0;
	break;

	/*
	 * horizontal motion
	 */
    case 'l':
	start_column = min(start_column + 1, MAXCOLUMN - 1);
	break;
    case 'h':
	start_column = max(start_column - 1, 0);
	break;
    case 'L':
	start_column = min(start_column + ((COLS - 2) / 2), MAXCOLUMN - 1);
	break;
    case 'H':
	start_column = max(start_column - ((COLS - 2) / 2), 0);
	break;
    case ']':
    case '\t':
	start_column = min(start_column + 8, MAXCOLUMN - 1);
	break;
    case '[':
    case '\b':
	start_column = max(start_column - 8, 0);
	break;
    case '>':
	start_column = min(start_column + (COLS - 2), MAXCOLUMN - 1);
	break;
    case '<':
	start_column = max(start_column - (COLS - 2), 0);
	break;
    case '{':
	start_column = 0;
	break;

	/*
	 * quit
	 */
    case 'q':
    case 'Q':
	die(0);
	break;

    default:
	return RSLT_ERROR;

    }

    prefix = 0;
    return RSLT_REDRAW;
}

const char *helpmsg[] = {
    " Scroll:                                         ",
    "            1-char    half-win  full-win  8-char ",
    "   UP       k, -      u, ^u     b, ^b            ",
    "   DOWN     j, RET    d, ^d     f, ^f            ",
    "   RIGHT    l         L         >         ]      ",
    "   LEFT     h         H         <         [      ",
    "                                                 ",
    "   g        goto top or prefix number line       ",
    "                                                 ",
    " Others:                                         ",
    "   space    update buffer                        ",
    "   0..9     prefix number argument               ",
    "   r        reverse character                    ",
    "   e        reverse entire line                  ",
    "   w        reverse word                         ",
    "   t        toggle reverse mode                  ",
    "   i        set interval for prefix number       ",
    "   p        pause and restart                    ",
    "   ?        show this message                    ",
    "   q        quit                                 ",
    (char*) 0,
};

void showhelp()
{
    size_t length = 0;
    int lines;
    int x, y;
    int i;
    const char *cp;
    WINDOW *helpwin;
    const char *cont_msg = " Hit any key to continue ";

    for (lines = 0; (cp = helpmsg[lines]) != (char*) 0; lines++)
	if (strlen(cp) > length)
	    length = strlen(cp);

    x = ((COLS - length - 2) + 1) / 2;
    y = ((LINES - lines - 2) + 1) / 2;

    if (x < 0 || y < 0) {
	fprintf(stderr, "\007");
	return;
    }

    helpwin = newwin(lines + 2, length + 2, y, x);

    wclear(helpwin);
    box(helpwin, '|', '-');

    for (i = 0; i < lines; i++) {
	wmove(helpwin, i + 1, 1);
	waddstr(helpwin, helpmsg[i]);
    }

    wmove(helpwin, lines + 1, length + 1 - strlen(cont_msg));
    wstandout(helpwin);
    waddstr(helpwin, cont_msg);
    wstandend(helpwin);

    wrefresh(helpwin);
    (void) wgetch(helpwin);
    werase(helpwin);
    wrefresh(helpwin);
    delwin(helpwin);
}

void untabify(buf, maxlen)
    char *buf;
    int maxlen;
{
    int tabstop = 8;
    char *p = buf;

    for (p = buf; *p; p++) {
	if (*p != '\t')
	    continue;
	else {
	    int spaces = tabstop - ((p - buf) % tabstop);

	    /*
	     * XXX: check the length!!!
	     */
	    maxlen = maxlen;
	    bcopy(p + 1, p + spaces, strlen(p + 1) + 1);
	    memset(p, ' ', spaces);
	}
    }
}

void die(int notused)
{
    erase(); 
    refresh(); 
    endwin();
    exit(0);
} 

void usage(name, optstr)
    char *name;
    const char *optstr;
{
    fprintf(stderr, "Usage: %s ", name);
    while (*optstr) {
	fprintf(stderr, "[-%c", *optstr++);
	if (*optstr == ':') {
	    fprintf(stderr, " #");
	    optstr++;
	}
	fprintf(stderr, "] ");
    }
    fprintf(stderr, "command [args]\n");
}
