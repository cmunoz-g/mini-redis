NAME = mini-redis
NAME_CLIENT = mini-redis-client
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
SRC_CLIENT = redis_client.cc
OBJ = $(SRC:%.cc=$(OBJ_FOLDER)%.o)
OBJ_CLIENT = $(SRC_CLIENT:%.cc=$(OBJ_FOLDER)%.o)

$(OBJ_FOLDER)%.o: $(FOLDER)%.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(addprefix -I,$(INCLUDES)) -c $< -o $@

$(NAME): $(OBJ)
	$(CXX) -o $(NAME) $(OBJ)

$(NAME_CLIENT) : $(OBJ_CLIENT)
	$(CXX) -o $(NAME_CLIENT) $(OBJ_CLIENT)

all: $(NAME) $(NAME_CLIENT)

server: $(NAME)

client: $(NAME_CLIENT)

clean:
	@rm -rf $(OBJ_FOLDER)

fclean: clean
	@rm -f $(NAME) $(NAME_CLIENT)

re: fclean all

.PHONY = all clean fclean re