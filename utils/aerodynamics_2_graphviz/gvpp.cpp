#include "gvpp.hpp"
#include <fstream>
#include <deque>
#include <cctype>
#include <cstring>
#include <codecvt>
#include <string>
#include <locale>
#include <algorithm>
#define CHECK_CALL(CODE) if (!(CODE)) throw runtime_error("Call to 'CODE' failed")

#define ostreamtype std::basic_ostream<chartype>
#define strtype std::basic_string<chartype>
#define ElementI Element<chartype>
#define AbstractGraphI AbstractGraph<chartype>
#define GraphI Graph<chartype>
#define SubGraphI SubGraph<chartype>
#define NodeI Node<chartype>
#define EdgeI Edge<chartype>
#define GvAttributesI GvAttributes<chartype>
#define printLineI printLine<chartype>

namespace gvpp {
    using namespace std;

    #define indentChar (chartype)'\t'

    #define quoteChar (chartype)'"'

    template<>
    string strFromLit(const wchar_t *chars) {
        static wstring_convert<codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(wstring(chars));
    }

    template<>
    wstring strFromLit(const char *chars) {
        static wstring_convert<codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(string(chars));
    }

    template<class chartype>
    void NodeI::printTo(printLineI pl) const {
        pl.os << pl.indent << id;
        this->getAttributes().printTo(pl);
        pl.os << ";" << endl;
    }

    template<class chartype>
    void EdgeI::printTo(printLineI pl) const {
        pl.os << pl.indent << getFrom().getId()
            << (this->getParent().isDirected() ? "->" : "--") << getTo().getId()
            <<" ";
            this->getAttributes().printTo(pl);
            pl.os <<";" << endl;
    }

    template<class chartype>
    void GvAttributesI::printTo(printLineI pl) const {
        if (!this->empty()) {
            pl.os << "[";
            int i = 0;
            for (auto p : *this)
                pl.os << (i++ > 0 ? "," : "") << get<0>(p) <<"="<< get<1>(p);
            pl.os << "]";
        }
    }

    template<class chartype>
    void AbstractGraphI::printGraphTo(printLineI pl) const {
        pl.os << pl.indent << getKeyword() <<" "<< getName() << " {\n";
        printLineI npl = {pl.os, pl.indent + (chartype)'\t'};
        if (getAttributes(AttrType::GRAPH).size() > 0) {
            npl.os << npl.indent << "graph ";
            getAttributes(AttrType::GRAPH).printTo(npl);
            npl.os << endl;
        }
        if (getAttributes(AttrType::EDGE).size() > 0) {
            npl.os << npl.indent << "edge ";
            getAttributes(AttrType::EDGE).printTo(npl);
            npl.os << endl;
        }
        if (getAttributes(AttrType::NODE).size() > 0) {
            npl.os << npl.indent << "node ";
            getAttributes(AttrType::NODE).printTo(npl);
            npl.os << endl;
        }
        for (auto e : this->elements) {
            e->printTo(npl);
        }
        pl.os << pl.indent << "}" << endl;
    }

    template<class chartype>
    AbstractGraphI::~AbstractGraph() {
        for (auto e : this->elements)
            delete e;
    }


    template<class chartype>
    SubGraphI &AbstractGraphI::addSubGraph(strtype name, bool cluster, strtype lab) {
        if (cluster && name.find(strFromLit<chartype>("cluster_")) != 0)
            name = strFromLit<chartype>("cluster_") + name;
        if (lab.length() > 0)
            lab = quoteChar + lab + quoteChar;
        elements.push_back(new SubGraphI(*this, name, lab));
        return static_cast<SubGraphI&>(*elements.back());
    }

    template<class chartype>
    NodeI &AbstractGraphI::addNode(strtype id, strtype lab, bool forcenew) {
        if (nodes.find(id) != nodes.end()) {
            if (forcenew)
                throw runtime_error("Node ID conflict : "+ strFromLit<char>(id.c_str()));
            else
                return static_cast<NodeI&>(*nodes.at(id));
        }
        if (lab.length() > 0)
            lab = quoteChar + lab + quoteChar;
        elements.push_back(new NodeI(*this, id, lab));
        nodes[id] = static_cast<NodeI*>(elements.back());
        return static_cast<NodeI&>(*elements.back());
    }

    template<class chartype>
    EdgeI &AbstractGraphI::addEdge(NodeI &n1, NodeI &n2, strtype lab) {
        if (lab.length() > 0)
            lab = quoteChar + lab + quoteChar;
        elements.push_back(new EdgeI(*this, n1, n2, lab));
        return static_cast<EdgeI&>(*elements.back());
    }

    template<class chartype>
    bool AbstractGraphI::hasNode(strtype id) {
        return nodes.find(id) != nodes.end();
    }

    template<class chartype>
    NodeI &AbstractGraphI::getNode(strtype id) {
        return *nodes.at(id);
    }

    template<class chartype>
    ostreamtype &operator<<(ostreamtype &os, const GraphI &g) {
        g.printGraphTo({os, strtype()});
        return os;
    }

    template<class chartype>
    const GvAttributesI &AbstractGraphI::getAttrsT(AttrType t) const {
        switch(t) {
        case AttrType::NODE:
            return NAttrs;
        case AttrType::EDGE:
            return EAttrs;
        case AttrType::GRAPH:
            return GAttrs;
        default:
#ifdef __GNUC__
            __builtin_unreachable();
#else
            return GAttrs;
#endif
        }
    }

    template<class chartype>
    GvAttributesI &AbstractGraphI::getAttrsT(AttrType t) {
        switch(t) {
        case AttrType::NODE:
            return NAttrs;
        case AttrType::EDGE:
            return EAttrs;
        case AttrType::GRAPH:
            return GAttrs;
        default:
#ifdef __GNUC__
            __builtin_unreachable();
#else
            return GAttrs;
#endif
        }
    }

    template<class chartype>
    strtype GraphI::getKeyword() const {
        return strFromLit<chartype>(directed ? "digraph" : "graph");
    }

    template<class chartype>
    strtype SubGraphI::getKeyword() const {
        return strFromLit<chartype>("subgraph");
    }

    template<class chartype>
    SubGraphI::SubGraph(AbstractGraphI &g, strtype name, strtype lab) :
            AbstractGraphI(name),
            ElementI(g) {
        if (lab.length() > 0)
            set(AttrType::GRAPH, strFromLit<chartype>("label"), lab);
    }

    string toCharString(basic_string<wchar_t> &&str) {
        wstring_convert<codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(str);
    }

    string toCharString(basic_string<char> &&str) {
        return str;
    }

    template<class chartype>
    int renderToFile(GraphI &g, string layout, string format, string file) {
        static const char *GUIFormats[] = {"x11", "xlib", "gtk"};
        static const locale loc;
        char *frmt = new char[format.length()+1];
        // Convert to lowercase
        for (size_t i = 0; i < format.length(); i ++)
            frmt[i] = use_facet<ctype<chartype>>(loc).tolower(format[i]);
        frmt[format.length()] = '\0';

        bool isGUI = false;
        for (const char* f : GUIFormats)
            if (strcmp(f, frmt) == 0)
                isGUI = true;

        if (!isGUI) {
            if (file.length() == 0) {
                file = string("-ooutput.")+ format;
            } else
                file = "-o"+ file;
        }
        delete[] frmt;

        basic_ostringstream<chartype> stream;
        stream << g;

        string buf = toCharString(stream.str());

        auto cmd = string("dot ")+ file +" -T"+ format+ " -K"+ layout;
        FILE *fd = fopen(cmd.c_str(), "w");
        fwrite(buf.c_str(), sizeof(chartype), buf.length(), fd);
        fflush(fd);
        return fclose(fd);
    }

    template class AbstractGraph<char>;
    template class Graph<char>;
    template class Element<char>;
    template class Node<char>;
    template class Edge<char>;
    template class SubGraph<char>;
    template basic_ostream<char> &operator<<
        (basic_ostream<char> &os, const Graph<char> &g);
    template int renderToFile
        (Graph<char> &g, string layout, string format, string file);

    template class AbstractGraph<wchar_t>;
    template class Graph<wchar_t>;
    template class Element<wchar_t>;
    template class Node<wchar_t>;
    template class Edge<wchar_t>;
    template class SubGraph<wchar_t>;
    template basic_ostream<wchar_t> &operator<<
        (basic_ostream<wchar_t> &os, const Graph<wchar_t> &g);
    template int renderToFile
        (Graph<wchar_t> &g, string layout, string format, std::string file);
}
