SRCS =	main.c \
		checksum.c \
		debugging.c

OBJS = ${SRCS:.c=.o}

NAME = ft_ping

CFLAGS		= -Wall -Wextra -Werror

all :		${NAME}

${NAME} :	${OBJS}
			gcc -o ${NAME} ${OBJS}
			
clean :
			rm -f ${OBJS}

fclean :	clean
			rm -f ${NAME}

re :		fclean all