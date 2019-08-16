# object files, auto generated from source files
OBJDIR := .o
$(shell mkdir -p $(OBJDIR) >/dev/null)

# dependency files, auto generated from source files
DEPDIR := .d
$(shell mkdir -p $(DEPDIR) >/dev/null)


CXX = g++
CC = $(CXX)
					 # -Ofast -funroll-loops -ffast-math -ftree-vectorize -flto \
					 # -march=native -mrecip
CXXFLAGS = -Werror -Wall -Wextra -Wconversion \
					 -Og \
					 -std=c++17 \
					 -g \
					 -IHyperLogLog\
					 -Idag\
					 -Idisjoint_set
CCFLAGS = $(CXXFLAGS)

LD = g++
#LDFLAGS = -pthread
LDLIBS = -static-libstdc++

DEPFLAGS = -MT $@ -MMD -MP -MF $(*:$(OBJDIR)/%=$(DEPDIR)/%).Td


COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
POSTCOMPILE = @mv -f $(*:$(OBJDIR)/%=$(DEPDIR)/%).Td $(*:$(OBJDIR)/%=$(DEPDIR)/%).d && touch $@

LINK.o = $(LD) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

all: \
	largest_out_component \
	largest_out_component_mobile \
	largest_out_component_transport \
	network_stats \
	network_stats_mobile \
	network_stats_transport \
	random_network \
	sample_bfs_mobile \
	sample_bfs_transport

tests: test_hll test_p_larger \
	test_deterministic_out_component_int \
	test_deterministic_out_component_double \
	test_deterministic_out_component_delyed

.PHONY: clean
clean:
	$(RM) -r $(OBJDIR) $(DEPDIR)

HyperLogLog/MurmurHash3.o:
	$(MAKE) -C HyperLogLog


# default is double timestamps, int32 nodes and 2^14bits of hll register
largest_out_component: $(OBJDIR)/largest_out_component.o\
	HyperLogLog/MurmurHash3.o
	$(LINK.o)

$(OBJDIR)/largest_out_component.o: CPPFLAGS +=\
	-DHLL_DENSE_PERC=14 \
	-D"NETWORK_TYPE=dag::undirected_temporal_network<uint32_t, double>"
$(OBJDIR)/largest_out_component.o: largest_out_component.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)


network_stats: $(OBJDIR)/network_stats.o\
	HyperLogLog/MurmurHash3.o
	$(LINK.o)

$(OBJDIR)/network_stats.o: CPPFLAGS +=\
	-DHLL_DENSE_PERC=14 \
	-D"NETWORK_TYPE=dag::undirected_temporal_network<uint32_t, double>"
$(OBJDIR)/network_stats.o: network_stats.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)



# directed version of default
largest_out_component_directed: $(OBJDIR)/largest_out_component_directed.o\
	HyperLogLog/MurmurHash3.o
	$(LINK.o)

$(OBJDIR)/largest_out_component_directed.o: CPPFLAGS +=\
	-DHLL_DENSE_PERC=14 \
	-D"NETWORK_TYPE=dag::directed_temporal_network<uint32_t, double>"
$(OBJDIR)/largest_out_component_directed.o: largest_out_component.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)


network_stats_directed: $(OBJDIR)/network_stats_directed.o\
	HyperLogLog/MurmurHash3.o
	$(LINK.o)

$(OBJDIR)/network_stats_directed.o: CPPFLAGS +=\
	-DHLL_DENSE_PERC=14 \
	-D"NETWORK_TYPE=dag::directed_temporal_network<uint32_t, double>"
$(OBJDIR)/network_stats_directed.o: network_stats.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)




# directed and delayed version of default
largest_out_component_directed_delayed: $(OBJDIR)/largest_out_component_directed_delayed.o\
	HyperLogLog/MurmurHash3.o
	$(LINK.o)

$(OBJDIR)/largest_out_component_directed_delayed.o: CPPFLAGS +=\
	-DHLL_DENSE_PERC=14 \
	-D"NETWORK_TYPE=dag::directed_delayed_temporal_network<uint32_t, double>"
$(OBJDIR)/largest_out_component_directed_delayed.o: largest_out_component.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)


network_stats_directed_delayed: $(OBJDIR)/network_stats_directed_delayed.o\
	HyperLogLog/MurmurHash3.o
	$(LINK.o)

$(OBJDIR)/network_stats_directed_delayed.o: CPPFLAGS +=\
	-DHLL_DENSE_PERC=14 \
	-D"NETWORK_TYPE=dag::directed_delayed_temporal_network<uint32_t, double>"
$(OBJDIR)/network_stats_directed_delayed.o: network_stats.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)





# mobile network has integer timestamps and is quite large
largest_out_component_mobile: $(OBJDIR)/largest_out_component_mobile.o\
	HyperLogLog/MurmurHash3.o
	$(LINK.o)

$(OBJDIR)/largest_out_component_mobile.o: CPPFLAGS +=\
	-DHLL_DENSE_PERC=10 \
	-D"NETWORK_TYPE=dag::undirected_temporal_network<uint32_t, uint32_t>"
$(OBJDIR)/largest_out_component_mobile.o: largest_out_component.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)


network_stats_mobile: $(OBJDIR)/network_stats_mobile.o\
	HyperLogLog/MurmurHash3.o
	$(LINK.o)

$(OBJDIR)/network_stats_mobile.o: CPPFLAGS +=\
	-DHLL_DENSE_PERC=10 \
	-D"NETWORK_TYPE=dag::undirected_temporal_network<uint32_t, uint32_t>"
$(OBJDIR)/network_stats_mobile.o: network_stats.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)



sample_bfs_mobile: $(OBJDIR)/sample_bfs_mobile.o\
	HyperLogLog/MurmurHash3.o
	$(LINK.o)

$(OBJDIR)/sample_bfs_mobile.o: CPPFLAGS +=\
	-D"NETWORK_TYPE=dag::undirected_temporal_network<uint32_t, uint32_t>"
$(OBJDIR)/sample_bfs_mobile.o: sample_bfs.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)



# twitter network is directed, has integer timestamps and is quite large
largest_out_component_twitter: $(OBJDIR)/largest_out_component_twitter.o\
	HyperLogLog/MurmurHash3.o
	$(LINK.o)

$(OBJDIR)/largest_out_component_twitter.o: CPPFLAGS +=\
	-DHLL_DENSE_PERC=10 \
	-D"NETWORK_TYPE=dag::directed_temporal_network<uint32_t, uint32_t>"
$(OBJDIR)/largest_out_component_twitter.o: largest_out_component.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)


network_stats_twitter: $(OBJDIR)/network_stats_twitter.o\
	HyperLogLog/MurmurHash3.o
	$(LINK.o)

$(OBJDIR)/network_stats_twitter.o: CPPFLAGS +=\
	-DHLL_DENSE_PERC=10 \
	-D"NETWORK_TYPE=dag::directed_temporal_network<uint32_t, uint32_t>"
$(OBJDIR)/network_stats_twitter.o: network_stats.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)



# directed and delayed version of default
largest_out_component_transport: $(OBJDIR)/largest_out_component_transport.o\
	HyperLogLog/MurmurHash3.o
	$(LINK.o)

$(OBJDIR)/largest_out_component_transport.o: CPPFLAGS +=\
	-DHLL_DENSE_PERC=14 \
	-D"NETWORK_TYPE=dag::directed_delayed_temporal_network<uint32_t, uint32_t>"
$(OBJDIR)/largest_out_component_transport.o: largest_out_component.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)


network_stats_transport: $(OBJDIR)/network_stats_transport.o\
	HyperLogLog/MurmurHash3.o
	$(LINK.o)

$(OBJDIR)/network_stats_transport.o: CPPFLAGS +=\
	-DHLL_DENSE_PERC=14 \
	-D"NETWORK_TYPE=dag::directed_delayed_temporal_network<uint32_t, uint32_t>"
$(OBJDIR)/network_stats_transport.o: network_stats.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)


sample_bfs_transport: $(OBJDIR)/sample_bfs_transport.o\
	HyperLogLog/MurmurHash3.o
	$(LINK.o)

$(OBJDIR)/sample_bfs_transport.o: CPPFLAGS +=\
	-D"NETWORK_TYPE=dag::directed_delayed_temporal_network<uint32_t, uint32_t>"
$(OBJDIR)/sample_bfs_transport.o: sample_bfs.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)




random_network: $(OBJDIR)/random_network.o HyperLogLog/MurmurHash3.o
	$(LINK.o)

$(OBJDIR)/random_network.o: random_network.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)



test_hll: $(OBJDIR)/test_hll.o HyperLogLog/MurmurHash3.o
	$(LINK.o)

$(OBJDIR)/test_hll.o: test_hll.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)




test_p_larger: $(OBJDIR)/test_p_larger.o HyperLogLog/MurmurHash3.o
	$(LINK.o)

$(OBJDIR)/test_p_larger.o: test_p_larger.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)




test_deterministic_out_component_int: $(OBJDIR)/test_deterministic_out_component_int.o HyperLogLog/MurmurHash3.o
	$(LINK.o)

$(OBJDIR)/test_deterministic_out_component_int.o: CPPFLAGS +=\
	-D"NETWORK_TYPE=dag::undirected_temporal_network<uint32_t, uint32_t>"
$(OBJDIR)/test_deterministic_out_component_int.o: test_deterministic_out_component.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)


test_deterministic_out_component_delyed: $(OBJDIR)/test_deterministic_out_component_delyed.o HyperLogLog/MurmurHash3.o
	$(LINK.o)

$(OBJDIR)/test_deterministic_out_component_delyed.o: CPPFLAGS +=\
	-D"NETWORK_TYPE=dag::directed_delayed_temporal_network<uint32_t, uint32_t>"
$(OBJDIR)/test_deterministic_out_component_delyed.o: test_deterministic_out_component.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)


test_deterministic_out_component_double: $(OBJDIR)/test_deterministic_out_component_double.o HyperLogLog/MurmurHash3.o
	$(LINK.o)

$(OBJDIR)/test_deterministic_out_component_double.o: CPPFLAGS +=\
	-D"NETWORK_TYPE=dag::undirected_temporal_network<uint32_t, double>"
$(OBJDIR)/test_deterministic_out_component_double.o: test_deterministic_out_component.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)





$(OBJDIR)/%.o : %.c
$(OBJDIR)/%.o : %.c $(DEPDIR)/%.d
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(OBJDIR)/%.o : %.cc
$(OBJDIR)/%.o : %.cc $(DEPDIR)/%.d
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(OBJDIR)/%.o : %.cpp
$(OBJDIR)/%.o : %.cpp $(DEPDIR)/%.d
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(OBJDIR)/%.o : %.cxx
$(OBJDIR)/%.o : %.cxx $(DEPDIR)/%.d
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

include $(wildcard $(DEPDIR)/*.d)
