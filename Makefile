NAME=rpi-simple-paramplot
CXXFLAGS=-Wall -std=c++0x
INCLUDES=-I/opt/vc/include \
		 -I/opt/vc/include/interface/vcos/pthreads \
		 -I/opt/vc/include/interface/vmcs_host/linux \
		 `pkg-config --cflags sdl`
LDFLAGS=-L/opt/vc/lib -lGLESv2 -lEGL `pkg-config --libs sdl`
SRCS=main.cpp graphics.cpp evaluator.cpp
OBJS=$(SRCS:%.cpp=%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

main.o: graphics.hpp evaluator.hpp exceptions.hpp
graphics.o: graphics.hpp exceptions.hpp
evaluator.o: evaluator.hpp

clean:
	rm -f $(OBJS)
	rm -f $(NAME)
