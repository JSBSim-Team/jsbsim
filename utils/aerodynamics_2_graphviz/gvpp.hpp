#include <iostream>
#include <sstream>
#include <tuple>
#include <list>
#include <map>
#include <functional>

namespace gvpp {
    enum class AttrType { NODE, EDGE, GRAPH };

    #define ostreamtype std::basic_ostream<chartype>
    #define strtype std::basic_string<chartype>

    #define TOWSTRING(x) L##x
    #define CWSTR(C,STR) gvpp::strFromLit<C>(STR, L##STR)

    template<class CharT>
    inline const CharT *strFromLit(const char *str, const wchar_t *wstr);

    template<class CharTo, class CharFrom>
    std::basic_string<CharTo> strFromLit(const CharFrom *str);

    template<>
    inline const char *strFromLit<char>(const char *str, const wchar_t *wstr) { return str; }
    template<>
    inline const wchar_t *strFromLit<wchar_t>(const char *str, const wchar_t *wstr) { return wstr; }

    template<>
    inline std::string strFromLit<char,char>(const char *str) { return str; }
    template<>
    inline std::wstring strFromLit<wchar_t,wchar_t>(const wchar_t *wstr) { return wstr; }

    template<class chartype>
    struct printLine {
        ostreamtype &os;
        strtype indent;
    };

    //template<class chartype> using printLine = struct printLine<chartype>;


    template<class chartype = char> class AbstractGraph;
    template<class chartype = char> class Element;
    template<class chartype = char> class Graph;
    template<class chartype = char> class SubGraph;
    template<class chartype = char> class Node;
    template<class chartype = char> class Edge;

    #define ElementT Element<chartype>
    #define AbstractGraphT AbstractGraph<chartype>
    #define GraphT Graph<chartype>
    #define SubGraphT SubGraph<chartype>
    #define NodeT Node<chartype>
    #define EdgeT Edge<chartype>
    #define GvAttributesT GvAttributes<chartype>
    #define printLineT printLine<chartype>

    template<class chartype>
    class GvAttributes : public std::map<strtype, strtype> {
    public:
        void printTo(printLineT pl) const;
    };

    template<class chartype>
    class AbstractGraph {
        friend class ElementT;
        friend class EdgeT;
        friend class GraphT;
        friend class SubGraphT;
    public:
        typedef chartype chartype_t;
        AbstractGraph(strtype name) : name(name) {}
        AbstractGraph(const AbstractGraph &g) = delete;
        ~AbstractGraph();
        virtual bool isDirected() const = 0;
        strtype getName() const { return name; }

        virtual const GvAttributesT &getAttributes(AttrType t) const { return getAttrsT(t); }
        virtual strtype get(AttrType t, strtype att) const { return getAttrsT(t).at(att); }
        virtual bool has(AttrType t, strtype att) const { return getAttrsT(t).find(att) != getAttrsT(t).end(); }
        virtual AbstractGraphT &set(AttrType t, strtype att, strtype val)
            { getAttrsT(t)[att] = val; return *this; }

        SubGraphT &addSubGraph(strtype name, bool cluster = false,
            strtype label = CWSTR(chartype,""));
        NodeT &addNode(strtype id, strtype lab = strtype(), bool forcenew=true);
        EdgeT &addEdge(NodeT &n1, NodeT &n2, strtype lab = strtype());
        bool hasNode(strtype id);
        NodeT &getNode(strtype id);        
        
        bool hasSubGraph(strtype id);
        SubGraphT &getSubGraph(strtype id);

        size_t size() { return elements.size(); }
        void printGraphTo(printLineT pl) const;

    protected:
        virtual GraphT &getRoot() = 0;
        GvAttributesT &getAttrsT(AttrType t);
        const GvAttributesT &getAttrsT(AttrType t) const;
    private:
        std::map<strtype, NodeT*> nodes;
        std::list<ElementT*> elements;
        strtype name;
        virtual strtype getKeyword() const = 0;
        GvAttributesT GAttrs, NAttrs, EAttrs;
    };

    template<class chartype>
    class Element {
        friend class AbstractGraphT;
    public:
        Element(AbstractGraphT& parent) : parent(parent) {}
        Element(const Element &e) = delete;
        virtual ~Element() {};
        AbstractGraphT &getParent() const { return parent; }
        virtual GraphT &getRootGraph() const { return parent.getRoot(); }
        virtual const GvAttributesT &getAttributes() const { return getAttrs(); }
        virtual strtype get(strtype att) const { return getAttrs().at(att); }
        virtual bool has(strtype att) const { return getAttrs().find(att) != getAttrs().end(); }
        virtual ElementT &set(strtype att, strtype val) { getAttrs()[att] = val; return *this; }
        strtype &operator[](const strtype& k) { return getAttrs()[k]; }
        strtype &operator[](strtype&& k) { return getAttrs()[k]; }
    protected:
        virtual void printTo(printLineT pl) const = 0;
        virtual GvAttributesT &getAttrs() { return attrs; }
        virtual const GvAttributesT &getAttrs() const { return attrs; }
    private:
        GvAttributesT attrs;
        AbstractGraphT &parent;
    };

    template<class chartype>
    class Node : public ElementT {
        friend class AbstractGraphT;
    public:
        const strtype getId() const { return id; }
        virtual Node &set(strtype att, strtype val) override
            { ElementT::set(att, val); return *this; }
    protected:
        void printTo(printLineT pl) const;
    private:
        Node(AbstractGraphT &g, strtype id, strtype lab) : ElementT(g), id(id) {
            if (lab.length() > 0)
            (*this)[CWSTR(chartype,"label")] = lab;
        }
        strtype id;
    };

    template<class chartype>
    class Edge : public std::pair<NodeT&, NodeT&>, public ElementT {
        friend class AbstractGraphT;
    public:
        Edge(AbstractGraphT &g, NodeT &n1, NodeT &n2, strtype lab) :
            std::pair<NodeT&,NodeT&>(n1, n2), ElementT(g)
            { if (lab.length() > 0) set(CWSTR(chartype,"label"), lab); }
        NodeT& getFrom() const { return std::get<0>(*this); }
        NodeT& getTo() const { return std::get<1>(*this); }
        virtual EdgeT &set(strtype att, strtype val) override
            { ElementT::set(att, val); return *this; }
    protected:
        void printTo(printLineT pl) const;
    };

    template<class chartype>
    class SubGraph : public AbstractGraphT, public ElementT {
        friend class AbstractGraphT;
    public:
        bool isCluster() const { return cluster; }
        bool isDirected() const override { return this->getParent().isDirected(); }
        const GvAttributesT &getAttributes(AttrType t) const override
            { return AbstractGraphT::getAttributes(t); }
        strtype get(AttrType t, strtype att) const override
            { return AbstractGraphT::get(t, att); }
        bool has(AttrType t, strtype att) const override
            { return this->getAttrsT(t).find(att) != this->getAttrsT(t).end(); }
        SubGraphT &set(AttrType t, strtype att, strtype val) override
            { AbstractGraphT::set(t, att, val); return *this; }

        const GvAttributesT &getAttributes() const override
            { return AbstractGraphT::getAttributes(AttrType::GRAPH); }
        strtype get(strtype att) const override
            { return AbstractGraphT::get(AttrType::GRAPH, att); }
        bool has(strtype att) const override
            { return AbstractGraphT::has(AttrType::GRAPH, att); }
        //SubGraph &set(strtype att, strtype val) override
        //    { AbstractGraphT::set(AttrType::GRAPH, att, val); return *this; }
    protected:
        GraphT &getRoot() override { return this->getRootGraph(); }
        GvAttributesT &getAttrs() override { return this->getAttrsT(AttrType::GRAPH); };
        const GvAttributesT &getAttrs() const override { return this->getAttrsT(AttrType::GRAPH); };
        void printTo(printLineT pl) const override { this->printGraphTo(pl); }
    private:
        SubGraph(AbstractGraphT &g, strtype name, strtype label);
        bool cluster;
        strtype getKeyword() const override;
    };

    template<class chartype>
    class Graph : public AbstractGraphT {
    public:
        Graph(bool dir = true, strtype name = strtype()) : AbstractGraphT(name),
            directed(dir) {}
        bool isDirected() const override { return directed; }
    protected:
        Graph &getRoot() override { return *this; }
    private:
        bool directed;
        strtype getKeyword() const override;
    };

    template<class chartype>
    ostreamtype  &operator<<(ostreamtype&, const GraphT&);
    template<class chartype>
    int renderToFile(GraphT &g, std::string layout, std::string format, std::string file = "");
    template<class chartype>
    inline int renderToScreen(GraphT &g, std::string layout) { return renderToFile(g, layout, "x11"); }
}

#undef ElementT
#undef AbstractGraphT
#undef GraphT
#undef SubGraphT
#undef NodeT
#undef EdgeT
#undef ostreamtype
#undef strtype

