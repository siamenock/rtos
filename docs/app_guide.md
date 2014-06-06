# PacketNgin OOP Programming Guide

Semih Kim (semih@gurum.cc)
2014-04-10

---

# Table of contents
[TOC]

# Class definition
Expression in Java
```
public class Human {
	public static int population;
    
    private int age;
    
    public int getAge() {
    	...
    }
    
    public void setAge(int age) {
    	...
    }
}
```

Expression in C
```
typedef struct _Human {
	int age;
} Human;

export int human_population;

void human_init(Human* this);
int human_get_age(Human* this);
void human_set_age(Human* this, int age);
```

# Class creation and deletion
Expression in Java
```
Human human = new Human();
human = null;	// Implicit deletion
```

Expression in C
```
Human* human = malloc(sizeof(Human));
bzero(human, sizeof(Human));
human_init(human);

free(human);
```

# Class inheritance
Expression in Java
```
public class Woman extends Human {
	private int secretOfWoman;
    
	Human giveBirth(Man man) {
    	...
    }
}
```

Expression in C
```
typedef struct _Woman {
	Human super;
    
    int secret_of_woman;
} Woman;

Human* woman_give_birth(Woman* this, Man* man);
```

# Type casting
Expression in Java
```
Human human = woman;	// Implicit type casting with error checking
```

Expression in C
```
Human* human = (Human*)woman;	// Explicit type casting no error checking
```
