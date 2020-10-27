#include <iostream>
#include <fstream>
#include <cmath>
#include <vector> 
#include <cassert>

#include<algorithm>
#include<functional> //Used to define addition of vectors

using namespace std;
class Linear_Convection_1d
{
    public:
    Linear_Convection_1d(double n_points, double cfl, string method);//input parameters
    //and which method to use - Lax-Wendroff or RK4
    void run_and_output_results();

    private:
    void make_grid();   // Grid is needed for writing output, and defining exact solution.
    void temporary_update_solution(const double factor, vector<double> &u, vector<double> &k); 
    //We'd use this to compute the rk4 slopes k_i's and temporarily store them in solution_new.
    //More precisely, it does k = solution_old + factor * u
    void rk4_solver();  //Gives the solution at next time step using RK4
    void lax_wendroff();//Gives the solutions at next time step using Lax-Wendroff
    void rhs_function(vector<double> &u, vector<double> &k); //This gives RHS of the system of ODEs
    //on which we apply RK4, and stores it in k.
    double coefficient = 1.0;

    double x_min = 0.0; double x_max = 1.0;

    vector<double> grid;

    vector<double> solution_old;   //Solution at previous step
    vector<double> solution_new;   //Solution at present step

    vector<double> solution_exact; //Exact solution at present time step

    double n_points; double h; double dt; double cfl;

    vector<double> k1; vector<double> k2; vector<double> k3;vector<double> k4; //slopes needed by rk4

    //This computes solution_new = u^{n+1} = u^n + dt/6 * (k1 + 2.0*k2 + 2.0*k3 + k4)
    void compute_solution_new_rk4();

    string method;
};

Linear_Convection_1d::Linear_Convection_1d(double n_points, double cfl, string method):n_points(n_points)
            , cfl(cfl), method(method)
{
   h = (x_max - x_min)/n_points;
   dt = cfl * h / abs(coefficient);
   cout << "Spatial grid points are at gap h = "<< h << endl;
   cout << "Time step dt = "<<dt << endl;
   grid.resize(n_points); solution_old.resize(n_points); solution_new.resize(n_points); solution_exact.resize(n_points);
   cout << "Number of spatial grid points is " << n_points <<endl;
   k1.resize(n_points);k2.resize(n_points);k3.resize(n_points);k4.resize(n_points);
};

void Linear_Convection_1d::make_grid()
{
    for (int i = 0; i < n_points; i++)
    {
        grid[i] = x_min + i * h;
    };
};

//This computes the rhs of the system of ODEs on which we apply rk4.
void Linear_Convection_1d::rhs_function(vector<double> &u, vector<double> &k)
{
    if (u.size() != k.size() or k.size() != solution_old.size())
    {
        cout << "You have used rhs_function to do rhs_funciton(&u,&k)"
             <<" with inappropriately sized u,k. Size of your u is "<< u.size() <<", size of k is "<< k.size()
                 <<". Both the sizes should equal "<< solution_old.size() << endl;
                 assert(false);
    }
    k[0] = -coefficient * (u[1] - u[n_points - 1]) / (2 * h); //left end point
    for (int j = 1; j < n_points-1; j++)
    {
        k[j] = -coefficient * (u[j+1] - u[j-1]) / (2 * h);
    }
    k[n_points - 1] = -coefficient * (u[0] - u[n_points - 2]) / (2 * h); //right end point
};

//k = solution_old + factor*u
void Linear_Convection_1d::temporary_update_solution(const double factor, vector<double> &k0, vector<double> &k)
{
    if (k0.size() != k.size() or k.size() != solution_old.size())
    {
        cout << "You have used temporary_update_solution to do 'k = solution_old + factor * k0 "
                 << "with inappropriately sized k0,k. Size of your k0 is "<< k0.size() <<", size of k is "<< k.size()
                 <<". Both the sizes should equal "<< solution_old.size() << endl;
                 assert(false);
    }
    for(int i = 0; i < k.size(); i++)
    {
        k[i] = solution_old[i] + factor * k0[i]; 
    } //k = solution_old + factor * k0
}

void Linear_Convection_1d::lax_wendroff()
{
    for (int j = 0; j < n_points; ++j) //Loop over grid points
    {
        if (j== 0)  //Periodic boundary condition
        solution_new[j] = solution_old[j]
                        - 0.5 * cfl * (solution_old[j+1] - solution_old[n_points - 1])
                        + 0.5 * cfl * cfl * (solution_old[n_points-1] - 2 * solution_old[j] + solution_old[j+1]);
        else if (j > 0 && j < n_points - 1)
        solution_new[j] = solution_old[j]
                        - 0.5 * cfl * (solution_old[j+1] - solution_old[j-1])
                        + 0.5 * cfl * cfl * (solution_old[j-1] - 2 * solution_old[j] + solution_old[j+1]);
        else if (j == n_points - 1) //periodic boundary condition
        solution_new[j] = solution_old[j]
                        - 0.5 * cfl * (solution_old[0] - solution_old[j-1])
                        + 0.5 * cfl * cfl * (solution_old[j-1] - 2 * solution_old[j] + solution_old[0]);
                
    }
    solution_old = solution_new;
};

//This computes solution_new = u^{n+1} = u^n + dt/6 * (k1 + 2.0*k2 + 2.0*k3 + k4)
void Linear_Convection_1d::compute_solution_new_rk4()
{
    for (int i = 0; i < n_points; i++)
        solution_new[i] = solution_old[i] + dt/6 * (k1[i] + 2.0*k2[i] + 2.0*k3[i] + k4[i]);
}

void Linear_Convection_1d::rk4_solver()
{       
        //This innocent step was what was causing trouble
        /* solution_old = solution_new; */
        
        //k1 = rhs_function(solution_old)
        rhs_function(solution_old, k1);
        //Temporarily putting u^{n+1} = solution_new = solution_old + dt/2 * k1
        temporary_update_solution(dt/2,k1 ,solution_new);
        //So, computing k2 = rhs_function(u^n + dt/2 * k1)
        rhs_function(solution_new,k2);
        //Similarly, temporarily putting u^{n+1}=solution_new = u^n + dt/2 *k2
        temporary_update_solution(dt/2, k2, solution_new);
        //Computing k3 = rhs_function(solution_old + dt/2 *k2)
        rhs_function(solution_new,k3);
        //Temporarily putting u^{n+1} = solution_new = u^n + dt * k3
        temporary_update_solution(dt, k3, solution_new);
        //Computing k4 = rhs_function(solution_old + dt *k3)
        rhs_function(solution_new, k4);

        //This computes solution_new = u^{n+1} = u^n + dt/6 * (k1 + 2.0*k2 + 2.0*k3 + k4)
        compute_solution_new_rk4();
        solution_old = solution_new;
};

void Linear_Convection_1d::run_and_output_results()
{
    make_grid();
    vector<double> initial_data(n_points); vector<double> error(n_points);
    for (int i = 0; i< n_points; i++) //Setting initial data to be constant 1
    {
        initial_data[i] = sin( 2 * M_PI * i*h);
    }
    int n_iterations = 10;
    for (int iteration_number = 0; iteration_number < n_iterations; iteration_number++)
    {
        if (iteration_number == 0)
        {
            solution_old = initial_data;
            solution_exact = initial_data;
            string file_name = "initial_solution.txt";
            ofstream output_solution;
            output_solution.open (file_name);
            for (int j = 0; j < n_points; j++)
            {
            if (j>0)
            output_solution << "\n";
            output_solution << grid[j] << " " << solution_old[j];
            }
            output_solution.close();
            for (int l = 0; l< n_points; l++)
                error[l] = solution_old[l] - solution_exact[l];
            cout << "Error at time t = 0 is given by these vectors "<< endl ;
            for (int i = 0; i < n_points; i++)
            cout << error[i] << " ";
        }
        else
        {
            for (int j = 0; j < n_points; j++)
            {
                solution_exact[j] = sin ( 2 * M_PI * ( grid[j] - coefficient * dt * iteration_number) );
            }
            if (method == "rk4")
            rk4_solver();
            else if (method == "lw")
            lax_wendroff();
            else
            assert(false);
            string file_name = "solution_";
            file_name += to_string(iteration_number); file_name += ".txt";
            ofstream output_solution;
            output_solution.open (file_name);
            for (int j = 0; j < n_points; j++)
            {
            if (j>0)
            output_solution << "\n";
            output_solution << grid[j] << " " << solution_new[j];
            }
            output_solution.close();
            
            for (int l = 0; l< n_points; l++)
                error[l] = solution_new[l] - solution_exact[l];
            cout << "Error at time t = "<<  iteration_number * dt  << " is given by these vectors "<< endl ;
            for (int i = 0; i < n_points; i++)
            cout << error[i] << " ";
            cout << endl;
        }
    }
}

int main()
{
    string method;
    double n_points = 60; double cfl = 0.3; //This makes cfl = 0.3
    cout << "Please type 'lw' for Lax-Wendroff and 'rk4' for Runge-Kutta 4."<<endl;
    cin >> method;
    Linear_Convection_1d solver(n_points, cfl, method);
    solver.run_and_output_results();
}