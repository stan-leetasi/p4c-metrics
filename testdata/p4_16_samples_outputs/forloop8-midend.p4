@test_keep_opassign header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> frag_offset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdr_checksum;
    bit<32> src_addr;
    bit<32> dst_addr;
}

struct headers_t {
    ipv4_t ipv4;
}

struct empty_metadata_t {
}

control c(inout headers_t headers, inout empty_metadata_t user_meta) {
    @name("c.sum") bit<32> sum_0;
    @name("c.i") bit<8> i_0;
    @hidden action forloop8l41() {
        sum_0 += 32w1;
        i_0 -= 8w1;
    }
    @hidden action forloop8l31() {
        sum_0 = 32w1;
    }
    @hidden action forloop8l47() {
        headers.ipv4.src_addr = sum_0;
    }
    @hidden table tbl_forloop8l31 {
        actions = {
            forloop8l31();
        }
        const default_action = forloop8l31();
    }
    @hidden table tbl_forloop8l41 {
        actions = {
            forloop8l41();
        }
        const default_action = forloop8l41();
    }
    @hidden table tbl_forloop8l47 {
        actions = {
            forloop8l47();
        }
        const default_action = forloop8l47();
    }
    apply {
        tbl_forloop8l31.apply();
        @unroll for (i_0 = 8w20; i_0 != 8w0; i_0 -= 8w2) {
            if (i_0 == 8w64) {
                continue;
            } else {
                tbl_forloop8l41.apply();
            }
        }
        if (sum_0 == 32w10) {
            tbl_forloop8l47.apply();
        }
    }
}

control c_<H, M>(inout H h, inout M m);
package top<H, M>(c_<H, M> c);
top<headers_t, empty_metadata_t>(c()) main;
