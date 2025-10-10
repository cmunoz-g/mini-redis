all: server client

NAME_SERVER = mini-redis
NAME_CLIENT = mini-redis-client
CXX = g++
CXXFLAGS = -Wall -Werror -Wextra
FOLDER_SERVER = src/server
FOLDER_CLIENT = src/client
OBJ_FOLDER_SERVER = obj/server
OBJ_FOLDER_CLIENT =	obj/client
INCLUDES_SERVER = includes/server includes/server/data-structures
INCLUDES_CLIENT = includes/client
SRC_SERVER = buffer.cc commands.cc entry.cc main.cc protocol.cc \
	server.cc socket.cc threadpool.cc timer.cc utils.cc \
	data-structures/avltree.cc data-structures/dlist.cc \
	data-structures/hashtable.cc data-structures/heap.cc \
	data-structures/sortedset.cc
SRC_CLIENT = client_responses.cc client.cc main.cc
OBJ_SERVER = $(SRC_SERVER:%.cc=$(OBJ_FOLDER_SERVER)/%.o)
OBJ_CLIENT = $(SRC_CLIENT:%.cc=$(OBJ_FOLDER_CLIENT)/%.o)

$(OBJ_FOLDER_SERVER)%.o: $(FOLDER_SERVER)%.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(addprefix -I,$(INCLUDES_SERVER)) -c $< -o $@

$(OBJ_FOLDER_CLIENT)%.o: $(FOLDER_CLIENT)%.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(addprefix -I,$(INCLUDES_CLIENT)) -c $< -o $@

$(NAME_SERVER): $(OBJ_SERVER)
	$(CXX) -o $(NAME_SERVER) $(OBJ_SERVER)

$(NAME_CLIENT) : $(OBJ_CLIENT)
	$(CXX) -o $(NAME_CLIENT) $(OBJ_CLIENT)

server: $(NAME_SERVER)

client: $(NAME_CLIENT)

clean:
	@rm -rf obj $(OBJ_FOLDER_SERVER) $(OBJ_FOLDER_CLIENT)

fclean: clean
	@rm -f $(NAME_SERVER) $(NAME_CLIENT)

re: fclean all

.PHONY: all clean fclean re client server