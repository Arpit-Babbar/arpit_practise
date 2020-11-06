#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
#include <string>
#include <stdio.h>
#include <sys/time.h>

using namespace std;

double max_element(vector<double> &v)
{
    double a = 0.0;
    for (unsigned int i = 0; i < v.size(); i++)
        a = max(a, v[i]);
    return a;
}

void output_vectors_to_file(string file_name, vector<double> &grid,
                            vector<double> &solution_old, vector<double> &solution_exact)
{
    ofstream output_solution;
    output_solution.open(file_name);
    for (unsigned int j = 0; j < grid.size(); j++)
    {
        output_solution << grid[j] << " " << solution_old[j] << " " << solution_exact[j] << "\n";
    }
    output_solution.close();
}
void output_vectors_to_file(string file_name, vector<double> &grid, vector<double> &solution_old)
{
    ofstream output_solution;
    output_solution.open(file_name);
    for (unsigned int j = 0; j < grid.size(); j++)
    {
        output_solution << grid[j] << " " << solution_old[j] << "\n";
    }
    output_solution.close();
}
class Linear_Convection_1d
{
public:
    Linear_Convection_1d(const double n_points, const double cfl,
                         string method, const double running_time,
                         int initial_data_indicator); //input parameters
    //and which method to use - Lax-Wendroff or RK4
    void run(); //true when output is to be given, and false when it doesn't;
    void output_final_error();

    vector<double> get_error();


private:
    void make_grid(); // Grid is needed for writing output, and defining exact solution.
    void set_initial_data();
    void temporary_update_solution(const double factor, const vector<double> &u);
    //We'd use this to compute the rk4 slopes k_i's and temporarily store them in solution_new.
    //More precisely, it does k = solution_old + factor * u
    void temporary_update_solution(const double factor0, const vector<double> &u0,
                                   const double factor1, const vector<double> &u1);
    void rk4_solver(); //Gives the solution at next time step using RK4
    void rk3_solver();
    void rk2_solver();
    void lax_wendroff();                                           //Computes next time step soln by Lax-Wendroff
    void rhs_function(const vector<double> &u, vector<double> &k); //This gives RHS of the system of ODEs
    double hat_function(double grid_point);
    double step_function(double grid_point); //Functions for initial data.
                                             //on which we apply RK4, and stores it in k.

    void evaluate_error_and_output_solution(const int time_step_number);

    double coefficient = 1.0;

    double x_min = -1.0;
    double x_max = 1.0;

    double interval_part(double); //If a point is not in the interval [x_min,x_max], this function sends it there
    //by translating in x_max-x_min sized steps.

    vector<double> grid;

    vector<double> solution_old; //Solution at previous step
    vector<double> solution_new; //Solution at present step

    vector<double> solution_exact; //Exact solution at present time step

    vector<double> initial_data;

    vector<double> error; //This will store the maximum error at a grid point in all time-steps
    double l2_error;

    //h denotes the spatial distance between grid points
    double n_points, h, dt, cfl, running_time; //h = 1/n_points just included for easy typing
    double t = 0;

    //slopes needed by rk4
    vector<double> k1, k2, k3, k4;

    //This computes solution_new = u^{n+1} = u^n + dt/6 * (k1 + 2.0*k2 + 2.0*k3 + k4)
    void compute_solution_new_using_ki(int);

    string method;
    int initial_data_indicator;
    int output_indicator = 0;
};

Linear_Convection_1d::Linear_Convection_1d(double n_points, double cfl, string method,
                                           double running_time, int initial_data_indicator)
                                          :n_points(n_points), cfl(cfl),
                                           running_time(running_time), method(method),
                                           initial_data_indicator(initial_data_indicator)
{
    h = (x_max - x_min) / (n_points);
    dt = cfl * h / abs(coefficient);
    cout << "h = " << h << endl;
    cout << "dt = " << dt << endl;
    grid.resize(n_points);
    initial_data.resize(n_points);
    error.resize(n_points);
    solution_old.resize(n_points);
    solution_new.resize(n_points);
    solution_exact.resize(n_points);
    cout << "h =  " << n_points << endl;
    k1.resize(n_points);
    k2.resize(n_points);
    k3.resize(n_points);
    k4.resize(n_points);
}

void Linear_Convection_1d::make_grid()
{
    for (int i = 0; i < n_points; i++)
    {
        grid[i] = x_min + i * h;
    }
}

void Linear_Convection_1d::set_initial_data()
{
    for (int i = 0; i < n_points; i++)
    {
        switch (initial_data_indicator)
        {
        case 0:
            initial_data[i] = sin(2 * M_PI * grid[i] / (x_max - x_min));
            break;
        case 1:
            initial_data[i] = hat_function(grid[i]);
            break;
        case 2:
            initial_data[i] = step_function(grid[i]);
            break;
        default:
            cout << "You entered the wrong initial_data_indicator ";
            assert(false);
        }
    }
}

//This computes the rhs of the system of ODEs on which we apply rk4.
void Linear_Convection_1d::rhs_function(const vector<double> &u, vector<double> &k)
{
    if (u.size() != k.size() || k.size() != solution_old.size())
    {
        cout << "You have used rhs_function to do rhs_funciton(&u,&k)"
             << " with inappropriately sized u,k. Size of your u is " << u.size() << ", size of k is " << k.size()
             << ". Both the sizes should equal " << solution_old.size() << endl;
        assert(false);
    }
    k[0] = -(u[1] - u[n_points - 1]) / (2.0 * h); //left end point
    for (int j = 1; j < n_points - 1; j++)
    {
        k[j] = -(u[j + 1] - u[j - 1]) / (2.0 * h);
    }
    k[n_points - 1] = -(u[0] - u[n_points - 2]) / (2.0 * h); //right end point
}

//k = solution_old + factor*u
void Linear_Convection_1d::temporary_update_solution(const double factor, const vector<double> &k0)
{
    if (k0.size() != solution_new.size() || solution_new.size() != solution_old.size())
    {
        cout << "You have used temporary_update_solution to do 'k = solution_old + factor * k0 ";
        cout << "with inappropriately sized k0,k. Size of your k0 is " << k0.size();
        cout << ", size of k is " << solution_new.size()
             << ". Both the sizes should equal " << solution_old.size() << endl;
        assert(false);
    }
    for (int i = 0; i < solution_new.size(); i++)
    {
        solution_new[i] = solution_old[i] + factor * k0[i];
    } //k = solution_old + factor * k0
}
void Linear_Convection_1d::temporary_update_solution(const double factor0, const vector<double> &k0,
                                                     const double factor1, const vector<double> &k1)
{
    if (k0.size() != solution_new.size() || solution_new.size() != solution_old.size())
    {
        cout << "You have used temporary_update_solution to do 'k = solution_old + factor * k0 ";
        cout << "with inappropriately sized k0,k. Size of your k0 is " << k0.size();
        cout << ", size of k is " << solution_new.size();
        cout << ". Both the sizes should equal " << solution_old.size() << endl;
        assert(false);
    }
    for (int i = 0; i < solution_new.size(); i++)
    {
        solution_new[i] = solution_old[i] + factor0 * k0[i] + factor1 * k1[i];
    } //k = solution_old + factor * k0
}

void Linear_Convection_1d::lax_wendroff()
{
    solution_new[0] = solution_old[0] 
                     - 0.5 * cfl * (solution_old[1] - solution_old[n_points - 1])
                     + 0.5 * cfl * cfl * (solution_old[n_points - 1] - 2.0 * solution_old[0] + solution_old[1]);
    for (int j = 1; j < n_points - 1; ++j) //Loop over grid points
    {
        solution_new[j] = solution_old[j] 
                          - 0.5 * cfl * (solution_old[j + 1] - solution_old[j - 1]) 
                          + 0.5 * cfl * cfl * (solution_old[j - 1] - 2.0 * solution_old[j] + solution_old[j + 1]);
    }
    solution_new[n_points - 1] = solution_old[n_points - 1] 
                                 - 0.5 * cfl * (solution_old[0] - solution_old[n_points - 2])
                                 + 0.5 * cfl * cfl * (solution_old[n_points - 2] 
                                 - 2 * solution_old[n_points - 1] + solution_old[0]);
}

void Linear_Convection_1d::rk4_solver()
{
    //This innocent step was what was causing trouble
    /* solution_old = solution_new; */

    //k1 = rhs_function(solution_old)
    rhs_function(solution_old, k1);
    //To save memory, we are going to temporarily store some things in solution_new
    //, which is supposed to be u^{n+1}
    //Temporarily putting 'u^{n+1}' = solution_new = solution_old + dt/2 * k1
    temporary_update_solution(dt / 2, k1);
    //So, computing k2 = rhs_function(u^n + dt/2 * k1)
    rhs_function(solution_new, k2);
    //Similarly, temporarily putting 'u^{n+1}'=solution_new = u^n + dt/2 *k2
    temporary_update_solution(dt / 2, k2);
    //Computing k3 = rhs_function(solution_old + dt/2 *k2)
    rhs_function(solution_new, k3);
    //Temporarily putting 'u^{n+1}' = solution_new = u^n + dt * k3
    temporary_update_solution(dt, k3);
    //Computing k4 = rhs_function(solution_old + dt *k3)
    rhs_function(solution_new, k4);

    //This computes solution_new = u^{n+1} = u^n + dt/6 * (k1 + 2.0*k2 + 2.0*k3 + k4)
    for (int i = 0; i < n_points; i++)
        solution_new[i] = solution_old[i] + dt / 6 * (k1[i] + 2.0 * k2[i] + 2.0 * k3[i] + k4[i]);
}

void Linear_Convection_1d::rk3_solver()
{
    //This innocent step was what was causing trouble
    /* solution_old = solution_new; */

    //k1 = rhs_function(solution_old)
    rhs_function(solution_old, k1);
    //Temporarily putting u^{n+1} = solution_new = solution_old + dt/2 * k1
    temporary_update_solution(dt / 2, k1);
    //So, computing k2 = rhs_function(u^n + dt/2 * k1)
    rhs_function(solution_new, k2);
    //Similarly, temporarily putting u^{n+1}=solution_new = u^n -k1 * dt + 2 k2 * dt
    temporary_update_solution(-dt, k1, 2 * dt, k2);
    //Computing k3 = rhs_function(solution_old - dt k1 + 2 dt k2)
    rhs_function(solution_new, k3);

    //This computes solution_new = u^{n+1} = u^n + dt/6 * (k1 + 2.0*k2 + 2.0*k3 + k4)
    for (int i = 0; i < n_points; i++)
        solution_new[i] = solution_old[i] + dt / 6 * (k1[i] + 4.0 * k2[i] + k3[i]);
}

void Linear_Convection_1d::rk2_solver()
{
    //This innocent step was what was causing trouble
    /* solution_old = solution_new; */

    //k1 = rhs_function(solution_old)
    rhs_function(solution_old, k1);
    //Temporarily putting u^{n+1} = solution_new = solution_old + dt * k1
    temporary_update_solution(dt, k1);
    //So, computing k2 = rhs_function(u^n + dt * k1)
    rhs_function(solution_new, k2);

    //This computes solution_new = u^{n+1} = u^n + dt/6 * (k1 + 2.0*k2 + 2.0*k3 + k4)
    for (int i = 0; i < n_points; i++)
        solution_new[i] = solution_old[i] + dt / 2 * (k1[i] + k2[i]);
}

void Linear_Convection_1d::evaluate_error_and_output_solution(int time_step_number)
{
    if (time_step_number == 0)
        solution_exact = initial_data;
    else
    {
        switch (initial_data_indicator)
        {
        case 0:
            for (int j = 0; j < n_points; j++)
            {
                solution_exact[j] = sin(2 * M_PI * (grid[j] - t) / (x_max - x_min));
            }
            break;
        case 1:
            for (int j = 0; j < n_points; j++)
            {
                solution_exact[j] = hat_function(grid[j] - t);
            }
            break;
        case 2:
            for (int j = 0; j < n_points; j++)
            {
                solution_exact[j] = step_function(grid[j] - t);
            }
            break;
        default:
            cout << "You entered the wrong initial_data_indicator ";
            assert(false);
        }
    }
    for (int j = 0; j < n_points; j++)
    {
        error[j] = abs(solution_new[j] - solution_exact[j]);
    }
    string solution_file_name = "solution_";
    solution_file_name += to_string(time_step_number) + "_" + method + ".txt";
    output_vectors_to_file(solution_file_name, grid, solution_new, solution_exact);
}

void Linear_Convection_1d::output_final_error()
{
    string error_file_name = "finalerror_";
    error_file_name += method + ".txt";
    output_vectors_to_file(error_file_name, grid, error);
}

void Linear_Convection_1d::run()
{
    make_grid();
    set_initial_data();
    solution_old = initial_data;
    int time_step_number = 0; //The bug I had made here was that I was running the solver
    //even for time_step_number = 0, which is wrong.
    evaluate_error_and_output_solution(time_step_number);
    while (t < running_time)
    {
        time_step_number += 1;
        t = t + dt;
        if (method == "rk4")
            rk4_solver();
        else if (method == "rk3")
            rk3_solver();
        else if (method == "rk2")
            rk2_solver();
        else if (method == "lw")
            lax_wendroff();
        else
            assert(false);
        solution_old = solution_new;
        evaluate_error_and_output_solution(time_step_number);
    }
    cout << "For n_points = " << n_points<<" we took " << time_step_number << " steps." << endl;
}

void run_and_get_output(double n_points, double cfl,
                        string method, double running_time,
                        int initial_data_indicator, double tolerance,
                        int max_refinements)
{
    double h = 1/n_points;
    ofstream error_vs_h;
    error_vs_h.open("error_vs_h.txt");
    if (tolerance == 0)
    {
        n_points = 40.0;
        cout << "By entering tolerance 0, you have chosen to do only one iteration with ";
        cout << n_points << " grid points" << endl;
        Linear_Convection_1d solver(n_points, cfl, method, running_time, initial_data_indicator);
        solver.run();
        cout << "The L_infty error is " << solver.get_error()[2] << endl;
        return;
    }
    Linear_Convection_1d solver(n_points, cfl, method, running_time, initial_data_indicator);
    solver.run();
    vector<double> linfty_vector(1);
    vector<double> l2_vector(1);
    vector<double> l1_vector(1);
    int refinement_level = 0;

    double testing_error = 1.0;//This will store the error that we'd iterate untill we reach tolerance.
    while (testing_error > tolerance && refinement_level < max_refinements)
    {
        refinement_level++;
        linfty_vector.push_back(solver.get_error()[2]);
        l2_vector.push_back(solver.get_error()[1]);
        l1_vector.push_back(solver.get_error()[0]);
        error_vs_h << h << " " << linfty_vector[refinement_level] << "\n";
        n_points = 2.0 * n_points;
        solver = Linear_Convection_1d(n_points, cfl, method, running_time, initial_data_indicator);
        if (refinement_level > 1)
        {
            cout << "Rate of Linfty convergence checked at refinement level " << refinement_level;
            cout << " is ";
            cout << abs(log(linfty_vector[refinement_level] / linfty_vector[refinement_level - 1])) / log(2.0);
            cout << endl;
            cout << "Rate of L2 convergence checked at refinemenet level " << refinement_level;
            cout << " is " << abs(log(l2_vector[refinement_level] / l2_vector[refinement_level - 1])) / log(2.0);
            cout << endl;
            cout << "Rate of L1 convergence checked at refinemenet level " << refinement_level;
            cout << " is " << abs(log(l1_vector[refinement_level] / l1_vector[refinement_level - 1])) / log(2.0);
            cout << endl;
        }
        struct timeval begin, end;
        gettimeofday(&begin, 0);
        solver.run();
        gettimeofday(&end, 0);
        long seconds = end.tv_sec - begin.tv_sec;
        long microseconds = end.tv_usec - begin.tv_usec;
        double elapsed = seconds + microseconds * 1e-6;
        cout << "Time taken by this iteration is " << elapsed << " seconds." << endl;
        if (initial_data_indicator == 0)
            testing_error = solver.get_error()[2];
        else
            testing_error = solver.get_error()[0];
    }
    error_vs_h.close();
    solver.output_final_error();
    cout << "After " << refinement_level << " refinements, l_infty error = ";
    cout << linfty_vector[refinement_level] << ", when the tolerance was " << tolerance << endl;
    cout << "The L2 error is " << l2_vector[refinement_level] << endl
         << endl;
}

double Linear_Convection_1d::interval_part(double x)
{
    if (x > x_max)
        return x - ceil((x_max - x_min) * (x - x_max)) / (x_max - x_min);
    else if (x < x_min)
        return x + ceil((x_max - x_min) * (x_min - x)) / (x_max - x_min);
    else
        return x;
}

double Linear_Convection_1d::hat_function(double grid_point)
{
    double value;
    grid_point = interval_part(grid_point);
    if (grid_point < -0.5 || grid_point > 0.5)
        value = 0;
    else if (grid_point <= 0.0)
        value = grid_point + 0.5;
    else if (grid_point >= 0.0)
        value = 0.5 - grid_point;
    return value;
}

double Linear_Convection_1d::step_function(double grid_point)
{
    double value;
    grid_point = interval_part(grid_point);
    if (grid_point < x_min + (x_max - x_min) / 4.0 || grid_point > x_max - (x_max - x_min) / 4.0)
        value = 0;
    else
        value = 1.0;
    return value;
}
vector<double> Linear_Convection_1d::get_error()
{
    vector<double> error_vector = {0.0,0.0,0.0};
    for (int j = 0; j < n_points; j++) //compute L^1 error
    {
        error_vector[0] = error_vector[0] + error[j]  * h;
    }
    for (int j = 0; j < n_points; j++) //compute L^2 error
    {
        error_vector[1] = error_vector[1] + error[j] * error[j]  * h;
    }
    error_vector[1] = pow(error_vector[1], 0.5);
    for (int j = 0; j < n_points; j++) //compute L^infty
    {
        error_vector[2] = max(error_vector[2], error[j]);
    }
    return error_vector;
}

int main(int argc, char **argv)
{
    if (argc != 7)
    {
        cout << "Incorrect format, use" << endl;
        cout << "./output lw/rk4/rk3/rk2(for the respective method) cfl running_time 0/1/2(for ";
        cout << "sine/hat/discts initial data) tolerance max_refinements" << endl;
        assert(false);
    }
    string method = argv[1];
    cout << "Method = " << method << endl;
    double n_points = 15.0;
    double cfl = stod(argv[2]);
    cout << "CFL = " << cfl << endl;
    double running_time = stod(argv[3]);
    cout << "running_time = " << running_time << endl;
    int initial_data_indicator = stoi(argv[4]);
    cout << "initial_data_indicator = " << initial_data_indicator << endl;
    double tolerance = stod(argv[5]);
    cout << "Tolerance = "<< tolerance << endl;
    int max_refinements = stoi(argv[6]);
    cout << "Max refinements = " << max_refinements <<endl;
    run_and_get_output(n_points, cfl, method, running_time, initial_data_indicator, tolerance, max_refinements);
}
