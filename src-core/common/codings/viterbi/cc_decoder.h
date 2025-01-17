/* -*- c++ -*- */
/*
 * Copyright 2013-2014 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include "cc_common.h"
#include <volk/volk_alloc.hh>
#include <map>
#include <string>

namespace fec
{
    namespace code
    {
        typedef void (*conv_kernel)(unsigned char *Y,
                                    unsigned char *X,
                                    unsigned char *syms,
                                    unsigned char *dec,
                                    unsigned int framebits,
                                    unsigned int excess,
                                    unsigned char *Branchtab);

        class cc_decoder_impl
        {
        public:
            // plug into the generic fec api
            int get_output_size();
            int get_input_size();
            int get_history();
            float get_shift();
            int get_input_item_size();
            const char *get_input_conversion();
            // const char* get_output_conversion();

        private:
            // everything else...
            void create_viterbi();
            int init_viterbi(struct v *vp, int starting_state);
            int init_viterbi_unbiased(struct v *vp);
            int update_viterbi_blk(unsigned char *syms, int nbits);
            int chainback_viterbi(unsigned char *data,
                                  unsigned int nbits,
                                  unsigned int endstate,
                                  unsigned int tailsize);
            int find_endstate();

            volk::vector<unsigned char> d_branchtab;
            unsigned char Partab[256];

            int d_ADDSHIFT;
            int d_SUBSHIFT;
            conv_kernel d_kernel;
            unsigned int d_max_frame_size;
            unsigned int d_frame_size;
            unsigned int d_k;
            unsigned int d_rate;
            std::vector<int> d_polys;
            cc_mode_t d_mode;
            int d_padding;

            struct v d_vp;
            volk::vector<unsigned char> d_managed_in;
            int d_numstates;
            int d_decision_t_size;
            int *d_start_state;
            int d_start_state_chaining;
            int d_start_state_nonchaining;
            int *d_end_state;
            int d_end_state_chaining;
            int d_end_state_nonchaining;
            unsigned int d_veclen;

            int parity(int x);
            int parityb(unsigned char x);
            void partab_init(void);

            // Buffering
            volk::vector<uint8_t> d_buffer;

        public:
            cc_decoder_impl(int frame_size,
                            int k,
                            int rate,
                            std::vector<int> polys,
                            int start_state = 0,
                            int end_state = -1,
                            cc_mode_t mode = CC_STREAMING,
                            bool padded = false);
            ~cc_decoder_impl();

            // Disable copy because of the raw pointers.
            cc_decoder_impl(const cc_decoder_impl &) = delete;
            cc_decoder_impl &operator=(const cc_decoder_impl &) = delete;

            void generic_work(void *inbuffer, void *outbuffer);
            int continuous_work(uint8_t *in, int size, uint8_t *out);
            void clear() { d_buffer.clear(); }

            bool set_frame_size(unsigned int frame_size);
            double rate();
        };

    } /* namespace code */
} /* namespace fec */
