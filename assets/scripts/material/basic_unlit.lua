
constant_buffer_bind = constant_buffer_bind_flag.ps
fail_safe_texture_index = 0

function make_constant_buffer(props)
   local cb = constant_buffer_builder.new([[
      bool use_emissive_map;
      float emissive_power;
   ]])


   cb:set("use_emissive_map", props:get_bool("UseEmissiveMap", false))
   cb:set("emissive_power", math2.exp2(props:get_float("EmissivePower", 0.0)))

   return cb:complete()
end

function fill_resource_vec(props, resource_props, resources)

   resources:add(resource_props["ColorMap"] or "$white")
   resources:add(resource_props["EmissiveMap"] or "")

end
