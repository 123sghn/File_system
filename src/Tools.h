#ifndef FILESYSTEM_TOOLS_H
#define FILESYSTEM_TOOLS_H

//彩色化输出printf方式
#define printf_grey(fmt, args...) \
    printf("\e[1;30m" fmt "\e[0m", ## args)

#define printf_red(fmt, args...) \
    printf("\e[1;31m" fmt "\e[0m", ## args)

#define printf_green(fmt, args...) \
    printf("\e[1;32m" fmt "\e[0m", ## args)

#define printf_yellow(fmt, args...) \
    printf("\e[1;33m" fmt "\e[0m", ## args)

#define printf_blue(fmt, args...) \
    printf("\e[1;34m" fmt "\e[0m", ## args)

#define printf_purple(fmt, args...) \
    printf("\e[1;35m" fmt "\e[0m", ## args)

#define printf_light_blue(fmt, args...) \
    printf("\e[1;36m" fmt "\e[0m", ## args)

#define printf_white(fmt, args...) \
    printf("\e[1;37m" fmt "\e[0m", ## args)

//彩色化输出cout方式
#define RESET "\033[0m"
#define BLACK "\033[30m"   /* Black */
#define RED "\033[31m"     /* Red */
#define GREEN "\033[32m"   /* Green */
#define YELLOW "\033[33m"  /* Yellow */
#define BLUE "\033[34m"    /* Blue */
#define MAGENTA "\033[35m" /* Magenta */
#define CYAN "\033[36m"    /* Cyan */
#define WHITE "\033[37m"   /* White */

#define BOLDBLACK "\033[1m\033[30m"   /* Bold Black */
#define BOLDRED "\033[1m\033[31m"     /* Bold Red */
#define BOLDGREEN "\033[1m\033[32m"   /* Bold Green */
#define BOLDYELLOW "\033[1m\033[33m"  /* Bold Yellow */
#define BOLDBLUE "\033[1m\033[34m"    /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m" /* Bold Magenta */
#define BOLDCYAN "\033[1m\033[36m"    /* Bold Cyan */
#define BOLDWHITE "\033[1m\033[37m"   /* Bold White */


#endif //FILESYSTEM_TOOLS_H
