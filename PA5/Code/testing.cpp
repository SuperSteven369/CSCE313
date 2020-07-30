#include <iostream>
#include <string>
using namespace std;

class Coord {
public:
    int x, y;
    Coord (int _x, int _y) {
        x = _x, y = _y;
    }
    string print() {
        return "(" + to_string(x) + "," + to_string(y) + ")";
    }
};

class Shape {
protected:
    Coord center;
public:
    Shape (Coord cent):center (cent) {
        cout << "Shape object constrcucting" << endl;
    }
    virtual ~Shape() {
        cout << "Shape object destroying" << endl;
    }
    virtual string print() = 0;
};

class Circle: public Shape {
private:
    int radius;
public:
    Circle (Coord _center, int _rad): Shape (_center), radius (_rad) {
        cout << "Circle object constrcucting" << endl;
    }
    ~Circle() {
        cout << "Circle object destroying" << endl;
    }
    string print() {
        return "Cicle is located in " + center.print() + " and has a radius: " + to_string(radius);
    }
};

class Rectangle: public Shape {
private:
    int width;
    int length;
public:
    Rectangle (Coord _center, int _w, int _l): Shape (_center), width (_w), length (_l) {
        cout << "Rectangle object constrcucting" << endl;
    }
    ~Rectangle() {
        cout << "Rectangle object destroying" << endl;
    }
    string print() {
        return "Rectangle is located in " + center.print() + " and has length: " + to_string(length) + " and width: " + to_string(width);
    }
};

int main (int ac, char ** av) {
    string desired_shape = av[1];
    Shape* s = NULL;
    if (desired_shape == "c") {
        s = new Circle (Coord (5, 5), 10);
    } else if (desired_shape == "r") {
        s = new Rectangle(Coord (10, 10), 5, 10);
    }
    cout << s->print() << endl;
    delete s;
    return 0;
}