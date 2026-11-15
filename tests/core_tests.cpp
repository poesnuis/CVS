#include "core/color.h"
#include "core/config.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <filesystem>
#include "reference_vectors.h"

int checks=0;
void check(bool b,const char* name) { ++checks; if(!b) throw std::runtime_error(name); }
void close(float a,float b,float e=1e-5f) { check(std::abs(a-b)<e,"numerical mismatch"); }
template<class F> void rejects(F f) { bool threw=false; try {f();} catch(...) {threw=true;} check(threw,"invalid input accepted"); }
int main() {
    using namespace cvs;
    try {
        close(decode_srgb(.5f),.21404114f); close(decode_srgb(.04045f),.003130805f);
        for(int i=0;i<256;++i) close(encode_srgb(decode_srgb(i/255.f)),i/255.f);
        // Published Machado 100% primary column; independently specified reference.
        const float endpoint[3][3]={{.152286f,.114503f,-.003882f},{.367322f,.280085f,-.011820f},{1.255528f,-.078411f,.004733f}};
        for(int d=0;d<3;++d) {
            auto m=machado_matrix(d,1); for(int i=0;i<3;++i) close(m.at(i,0),endpoint[d][i]);
            auto z=machado_matrix(d,0); for(int i=0;i<9;++i) close(z.v[i],identity.v[i]);
            auto l=machado_matrix(d,.4f),h=machado_matrix(d,.5f),mid=machado_matrix(d,.45f);
            for(int i=0;i<9;++i) close(mid.v[i],(l.v[i]+h.v[i])*.5f);
            auto b=brettel_matrices(d);
            auto w=mul(b.first,Vec3{1,1,1}); close(w.x,1); close(w.y,1); close(w.z,1);
            // A dichromat projection is idempotent; both planes meet at white.
            for(auto plane:{b.first,b.second}) {
                auto twice=mul(plane,plane); for(int i=0;i<9;++i) close(twice.v[i],plane.v[i],3e-5f);
            }
            check(dot({1,0,0},b.separator)*dot({0,0,1},b.separator)<0||dot({0,1,0},b.separator)*dot({0,0,1},b.separator)<0,"Brettel branches");
        }
        Settings gray; gray.preset=Preset::Achromatopsia;
        for(const auto& ref:referenceVectors) {
            Settings s;s.preset=static_cast<Preset>(ref.deficiency*2);s.model=static_cast<Model>(ref.model);s.severity=ref.severity;
            auto actual=transform_pixel(ref.input,make_transform(s));
            close(actual.x,ref.expected.x,2e-5f);close(actual.y,ref.expected.y,2e-5f);close(actual.z,ref.expected.z,2e-5f);
        }
        auto r=transform_pixel({1,0,0},make_transform(gray)); close(r.x,.4984399f); close(r.x,r.y); close(r.y,r.z);
        Settings custom; custom.preset=Preset::CustomLMS;
        auto lc=transform_pixel({.2f,.4f,.6f},make_transform(custom)); close(lc.x,.2f); close(lc.y,.4f); close(lc.z,.6f);
        for(int p=0;p<15;++p) for(auto model:{Model::Recommended,Model::Machado,Model::Brettel}) {
            Settings s; s.preset=static_cast<Preset>(p); s.model=model; s.severity=0;
            auto t=make_transform(s); auto x=transform_pixel({.2f,.5f,.8f},t); close(x.x,.2f); close(x.y,.5f); close(x.z,.8f);
            for(float sev:{.1f,.55f,1.f}) {
                s.severity=sev; t=make_transform(s);
                for(int a=0;a<=4;++a) for(int b=0;b<=4;++b) for(int c=0;c<=4;++c) {
                    auto y=transform_pixel({a/4.f,b/4.f,c/4.f},t);
                    check(std::isfinite(y.x)&&y.x>=0&&y.x<=1&&std::isfinite(y.y)&&y.y>=0&&y.y<=1&&std::isfinite(y.z)&&y.z>=0&&y.z<=1,"gamut/finite");
                }
            }
        }
        rejects([]{Settings s;s.severity=std::numeric_limits<float>::quiet_NaN();make_transform(s);});
        Config cfg; cfg.saved.push_back({"My preset",gray}); cfg.allMonitors=true; cfg.monitors={"DISPLAY1"};
        auto text=serialize(cfg); check(serialize(deserialize(text))==text,"JSON round trip");
        auto path=std::filesystem::temp_directory_path()/"cvs-test-config.json";
        save_config(path,cfg); cfg.settings.severity=.42f; save_config(path,cfg);
        close(load_config(path).settings.severity,.42f); std::filesystem::remove(path);
        rejects([]{deserialize("{\"schema\":2}");});
        rejects([]{deserialize("{\"schema\":1,\"settings\":{\"preset\":\"protanopia\",\"severity\":2}}");});
        rejects([]{deserialize("{\"schema\":1,\"settings\":{\"preset\":\"missing\"}}");});
        rejects([]{deserialize("{\"schema\":1,\"hotkeys\":[]}");});
        rejects([]{deserialize(std::string(1024*1024+1,' '));});
        auto hot=cfg.hotkeys; hot[4]={3,0x22}; validate_hotkeys(hot); hot[1]=hot[0]; rejects([&]{validate_hotkeys(hot);});
        std::cout << "PASS: " << checks << " assertions (transfer, matrices, severity, presets, configuration, validation)\n";
        return 0;
    } catch(const std::exception& e) { std::cerr << "FAIL after "<<checks<<": "<<e.what()<<'\n'; return 1; }
}
