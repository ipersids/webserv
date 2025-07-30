# Compilation variables
CXX		:= c++
CXXFLAGS	:= -Wall -Wextra -Werror -std=c++17 -MMD -MP
# -MMD generates dependency files (.d) that list all headers for source
# -MP adds phony targets for headers to prevent errors if headers are removed
HDRS		:= -Iincludes

# Directories
OBJ_DIR		:= obj
SRC_DIRS	:= srcs srcs/Config
VPATH		:= $(SRC_DIRS)

# Sources and objects
SRCS		:= utils.cpp data.cpp ConfigParser.cpp \
			HttpRequest.cpp \
			HttpRequestUtils.cpp \
			HttpRequestParser.cpp
SRC_MAIN	:= main.cpp

OBJS		:= $(addprefix $(OBJ_DIR)/, $(SRCS:.cpp=.o))
OBJS_MAIN	:= $(OBJ_DIR)/main.o

# Dependency files (for tracking header changes)
DEPS        	:= $(OBJS:.o=.d) $(OBJS_MAIN:.o=.d)

# Program name
NAME		:= webserv

# Rules
all: $(OBJ_DIR) $(NAME)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(HDRS) -c $< -o $@

$(NAME): $(OBJS) $(OBJS_MAIN)
	$(CXX) $(CXXFLAGS) $(OBJS) $(OBJS_MAIN) $(HDRS) -o $(NAME)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

run: all
	ulimit -n 100000 && ./webserv # default ulimit is 1000 we need to cover the max value for connections here

# Include the auto-generated dependency information
-include $(DEPS)

# Testing
LIB_NAME		:= libwebserv.a
TEST_NAME		:= test.out
TEST_SRCS		:= tests/test_main.cpp \
				tests/http-unit-tests/test_http_request.cpp \
				tests/http-unit-tests/test_http_request_parser.cpp
TEST_SRCS_WITH_PATHS	:= $(addprefix srcs/, $(SRCS)) $(TEST_SRCS)

test: $(OBJ_DIR) $(LIB_NAME)
	@echo "Building and running tests..."
	$(CXX) -Wall -Wextra -Werror -std=c++17 $(HDRS) $(TEST_SRCS) -L. -lwebserv -o $(TEST_NAME)
	./$(TEST_NAME)
	rm -f ./test.out $(LIB_NAME)

$(LIB_NAME): $(OBJS)
	ar rcs $(LIB_NAME) $(OBJS)

.PHONY: $(NAME) all clean fclean re run test
