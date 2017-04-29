
function ( find_source_tree out base_group base_dir pattern )

	set( lst )
	set( out_files )

	file( GLOB files "${base_dir}/${pattern}" )
	foreach ( f ${files} )
		if ( NOT IS_DIRECTORY "${f}" )
			list( APPEND out_files "${f}" )
		endif ( )
	endforeach ( )

	source_group( "${base_group}" FILES ${out_files} )

	file( GLOB subdirectories "${base_dir}/*" )
	foreach ( f ${subdirectories} )
		if ( IS_DIRECTORY "${f}" )
            set( subfiles )
			get_filename_component( name "${f}" NAME )
			find_source_tree( subfiles "${base_group}\\\\${name}" "${f}" "${pattern}" )
			list( APPEND out_files ${subfiles} )
		endif ( )
	endforeach ( )

	set( ${out} ${out_files} PARENT_SCOPE )

endfunction ( )

function ( make_relative out base_dir )

	set( out_files )

	foreach ( f ${ARGN} )
		file( RELATIVE_PATH relname ${base_dir} ${f} )
		list( APPEND out_files "${relname}" )
	endforeach ( )

	set( ${out} ${out_files} PARENT_SCOPE )

endfunction ( )
