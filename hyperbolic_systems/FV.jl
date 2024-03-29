module FV

using Grid
using LinearAlgebra
using OffsetArrays
using TickTock
using Plots
using UnPack
plotly(size = (750, 565)) # use gr as the plot backend, for its high performance
#-------------------------------------------------------------------------------
# Create a dictionary of problem description
#-------------------------------------------------------------------------------
function Problem(domain::Tuple{Float64,Float64},
                 nvar::Int64,
                 initial_value::Function,
                 boundary_value::Function,
                 boundary_condition::Any,
                 final_time::Float64)
Dict("domain" => domain,
     "nvar"   => nvar,
     "initial_value" => initial_value,
     "boundary_value" => boundary_value,
     "boundary_condition" => boundary_condition,
     "final_time" => final_time)
end

#-------------------------------------------------------------------------------
# Create a dictionary of parameters
#-------------------------------------------------------------------------------
function Parameters(grid_size::Int64,
                    cfl::Float64,
                    Ccfl::Float64,
                    save_time_interval::Float64)
@assert (cfl >= 0.0) "cfl must be >= 0.0"
@assert (save_time_interval >= 0.0) "save_time_interval must be >= 0.0"
Dict("grid_size" => grid_size,
     "cfl" => cfl,
     "Ccfl"=> Ccfl,
     "save_time_interval" => save_time_interval)
end

#-------------------------------------------------------------------------------
# Create a dictionary of scheme description
#-------------------------------------------------------------------------------
function Scheme(equation::Dict, numflux::String)
   numfluxes = equation["numfluxes"]
   Dict("numflux" => numfluxes[numflux], "numflux_ind"  => numflux)
end

#-------------------------------------------------------------------------------
# Create ghosted arrays with one layer of cells on both sides
#-------------------------------------------------------------------------------
gArray(nvar, nx) = OffsetArray(zeros(nvar, nx+2),
                              OffsetArrays.Origin(1, 0))

#-------------------------------------------------------------------------------
# Convert array to string
#-------------------------------------------------------------------------------
function array2string(arr)
   arr_string = "["
   n = size(arr)[1]
   for i=1:n-1
      arr_string = arr_string * string(arr[i]) * ","
   end
   arr_string = arr_string * string(arr[end]) * "]"
end

#-------------------------------------------------------------------------------
# Adjust dt to reach final time or the next time when solution has to be saved
#-------------------------------------------------------------------------------
function adjust_time_step(problem, param, dt, t)
   # Adjust to reach final time exactly
   final_time = problem["final_time"]
   save_time_interval = param["save_time_interval"]
   if t + dt > final_time
      dt = final_time - t
      return dt
   end

   # Adjust to reach next solution saving time
   if save_time_interval > 0.0
      next_save_time = ceil(t/save_time_interval) * save_time_interval
      # If t is not a plotting time, we check if the next time
      # would step over the plotting time to adjust dt
      if abs(t-next_save_time) > 1e-10 && t + dt - next_save_time > 1e-10
         dt = next_save_time - t
         return dt
      end
   end
   return dt
end

function compute_lam_dt(equation, scheme, param, grid, Ua)
   fprime  = equation["fprime"]
   eq      = equation["eq"]
   numflux = scheme["numflux_ind"]
   Ccfl    = param["Ccfl"]            # safety CFL


   nx, dx = grid.nx, grid.dx
   xc     = grid.xc
   lam = 0.0
   dt  = 1.0
   for i=1:nx
      ua   = Ua[:, i]
      if numflux in ["lax_friedrich", "rusanov","upwind"]
         lam0 = maximum(abs.(eigvals(fprime(xc[i], ua, eq))))
      else
         ρ, u, E = ua[1], ua[2]/ua[1], ua[3]    # density, velocity, energy
         p = (eq.γ - 1.0) * (E - 0.5*ρ*u^2)     # pressure
         c = sqrt(eq.γ*p/ρ)                     # sound speed
         lam0 = abs(u)+c
      end
      lam  = max(lam, lam0)
      dt   = min(dt, dx[i]/lam0)
   end
   if numflux == "lax_friedrich"
      return lam, dt
   else
      dt, lam = Ccfl*dt, Ccfl*lam
   end
   return lam, dt
end

function set_initial_value!(grid, U, problem)
   nx = grid.nx
   xc = grid.xc
   initial_value = problem["initial_value"]
   for i=1:nx
      U[:,i] = initial_value(xc[i])
   end
end

function update_ghost!(grid, U, problem)
   xmin, xmax = grid.domain
   nx = grid.nx
   initial_value = problem["initial_value"]
   if problem["boundary_condition"] == "Dirichlet"
      @views U[:, 0]   .= initial_value(xmin) # only for short time
      @views U[:,nx+1] .= U[:,nx]
   else
      @views U[:, 0]    .= U[:, nx]
      @views U[:, nx+1] .= U[:,1]
   end
   return nothing
end

function compute_residual!(equation, grid, lam, U, scheme, res)
   nx = grid.nx
   xf = grid.xf
   dx = grid.dx
   eq = equation["eq"]
   numflux = scheme["numflux"]
   dx0 =  OffsetArray(zeros(nx+2), OffsetArrays.Origin(0))
   dx0[1:nx] .= dx
   dx0[0] = dx0[nx+1] = 0.0 # redundant values
   res[:,:] .= 0.0 # Shouldn't we be able to avoid this?
                   # Something like this?
                   #      @views res[:, i-1] += f/ dx0[i-1]
                   #      @views res[:, i]   = f/ dx0[i]
   # loop over faces
   Uf = zeros(3)
   for i=1:nx+1
      @views Ul, Ur  = U[:,i-1], U[:,i]
      numflux(equation, lam, Ul, Ur, xf[i], Uf)
      @views res[:, i-1] += Uf/ dx0[i-1]
      @views res[:, i]   -= Uf/ dx0[i]
   end
end

function solve(equation, problem, scheme, param, plotters)
   tick()
   grid = make_grid(problem, param)
   initialize_plot = plotters["initialize_plot"]
   update_plot!    = plotters["update_plot!"]
   final_plot      = plotters["final_plot"]
   nvar = problem["nvar"]
   Tf = problem["final_time"]
   nx = grid.nx
   dx = grid.dx
   xf = grid.xf
   # Allocating variables
   U   = gArray(nvar, nx)
   Ue  = zeros(nvar, nx)
   res = gArray(nvar, nx) # dU/dt + res(U) = 0
   Ua  = U # ua is just Ua for this first order method,
           # storing for clarity
   set_initial_value!(grid, U, problem)
   it, t = 0, 0.0
   plt_data = initialize_plot(grid, problem, equation, scheme, U)
   while t < Tf
      lam, dt = compute_lam_dt(equation, scheme, param, grid, Ua)
      dt = adjust_time_step(problem, param, dt, t)
      update_ghost!(grid, U, problem)
      compute_residual!(equation, grid, lam, U, scheme, res)
      @. U -= dt*res
      t += dt; it += 1
      update_plot!(grid, problem, equation, scheme, U, t, it, param, plt_data)
   end
   final_plot(plt_data)
   tock()
   return plt_data
end

export Problem
export Parameters
export Scheme
export empty_func
export array2string
export solve

end