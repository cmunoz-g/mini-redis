NAME = mini-redis
CXX = g++
CXXFLAGS = -Wall -Werror -Wextra
FOLDER = src/
OBJ_FOLDER = obj/
INCLUDES = includes/ includes/data-structures
SRC = buffer.cc commands.cc entry.cc main.cc protocol.cc \
	server.cc socket.cc threadpool.cc timer.cc \
	data-structures/avltree.cc data-structures/dlist.cc \
	data-structures/hashtable.cc data-structures/heap.cc \
	data-structures/sortedset.cc
OBJ = $(SRC:%.cc=$(OBJ_FOLDER)%.o)

$(OBJ_FOLDER)%.o: $(FOLDER)%.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(addprefix -I,$(INCLUDES)) -c $< -o $@

$(NAME): $(OBJ)
	$(CXX) -o $(NAME) $(OBJ)

all: $(NAME)

clean:
	@rm -rf $(OBJ_FOLDER)

fclean: clean
	@rm -f $(NAME)

re: fclean all

.PHONY = all clean fclean re